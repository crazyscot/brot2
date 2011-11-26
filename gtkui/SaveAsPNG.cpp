/*
    SaveAsPNG: PNG export action/support for brot2
    Copyright (C) 2011 Ross Younger

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SaveAsPNG.h"
#include "Render.h"
#include "misc.h"

#include "MainWindow.h"
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/table.h>
#include <gtkmm/main.h>
#include <iostream>

#include <errno.h>
#include <stdio.h>
#include <string.h>

std::string SaveAsPNG::last_saved_dirname = "";

void SaveAsPNG::to_png(MainWindow *mw, unsigned rwidth, unsigned rheight,
		Plot2* plot, BasePalette* pal, int aafactor,
		std::string& filename)
{
	FILE *f;

	f = fopen(filename.c_str(), "wb");
	if (f==NULL) {
		std::ostringstream str;
		str << "Could not open file for writing: " << errno << " (" << strerror(errno) << ")";
		Util::alert(mw, str.str());
		return;
	}

	const char *errs = 0;
	if (0 != Render::save_as_png(f, rwidth, rheight, *plot,
				*pal, aafactor, &errs)) {
		Util::alert(mw, errs);
		return;
	}

	if (0==fclose(f)) {
		mw->get_progbar()->set_text("Successfully saved");
	} else {
		std::ostringstream str;
		str << "Error closing the save: " << errno << " (" << strerror(errno) << ")";
		Util::alert(mw, str.str());
	}
}

struct PNGProgressWindow: public Gtk::Window, Plot2::callback_t {
	MainWindow *parent;
	SaveAsPNG *job;
	Gtk::ProgressBar *progbar;

	PNGProgressWindow(MainWindow *p, SaveAsPNG* j) : parent(p), job(j) {
		set_transient_for(*parent);
		set_title("PNG export");
		Gtk::VBox* box = Gtk::manage(new Gtk::VBox());
		progbar = Gtk::manage(new Gtk::ProgressBar());
		box->pack_start(*progbar);
		progbar->set_text("Rendering pass 1...");

		add(*box);
		show_all();
	}

	virtual void plot_progress_minor(Plot2& UNUSED(plot), float workdone) {
		gdk_threads_enter();
		progbar->set_fraction(workdone);
		progbar->queue_draw();
		queue_draw();
		gdk_flush();
		gdk_threads_leave();
	}
	virtual void plot_progress_major(Plot2& UNUSED(plot), unsigned UNUSED(current_maxiter), std::string& commentary) {
		gdk_threads_enter();
		progbar->set_text(commentary);
		progbar->set_fraction(0.0);
		parent->queue_draw();
		gdk_flush();
		gdk_threads_leave();
	}
	virtual void plot_progress_complete(Plot2& UNUSED(plot)) {
		parent->queue_png(job);
	}
};

class FileChooserExtra : public Gtk::VBox {
	public:
		Gtk::CheckButton *resize, *antialias;
		Gtk::HBox *inner;
		Util::HandyEntry<int> *f_x, *f_y;

		FileChooserExtra(int init_x, int init_y) {
			Gtk::Label *lbl;

			resize = Gtk::manage(new Gtk::CheckButton("Save at a different size (requires re-render)"));
			resize->set_active(false);
			pack_start(*resize);
			resize->signal_toggled().connect(sigc::mem_fun(*this, &FileChooserExtra::do_toggle));

			inner = Gtk::manage(new Gtk::HBox());

			lbl = Gtk::manage(new Gtk::Label("X resolution", 0.9, 0.5));
			inner->pack_start(*lbl);
			f_x = Gtk::manage(new Util::HandyEntry<int>());
			f_x->set_activates_default(true);
			inner->pack_start(*f_x);

			lbl = Gtk::manage(new Gtk::Label("Y resolution", 0.9, 0.5));
			inner->pack_start(*lbl);
			f_y = Gtk::manage(new Util::HandyEntry<int>());
			f_y->set_activates_default(true);
			inner->pack_start(*f_y);

			antialias = Gtk::manage(new Gtk::CheckButton("Antialias"));
			antialias->set_active(false);
			inner->pack_start(*antialias);

			f_x->update(init_x);
			f_y->update(init_y);

			pack_start(*inner);
			show_all();
			resize->show();
			inner->hide();
		}

		void do_toggle() {
			if (resize->get_active())
				inner->show();
			else
				inner->hide();
		}

		bool x(int& xx) {
			return f_x->read(xx);
		}
		bool y(int& yy) {
			return f_y->read(yy);
		}
};

void SaveAsPNG::do_save(MainWindow *mw)
{
	std::string filename;
	int newx=0, newy=0;
	bool do_extra = false, do_antialias = false;

	{
		Gtk::FileChooserDialog dialog(*mw, "Save File", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
		dialog.set_do_overwrite_confirmation(true);
		dialog.add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
		dialog.add_button(Gtk::Stock::SAVE, Gtk::ResponseType::RESPONSE_ACCEPT);

		FileChooserExtra *extra = Gtk::manage(new FileChooserExtra(
					mw->get_rwidth(), mw->get_rheight() ));
		dialog.set_extra_widget(*extra);

		if (!last_saved_dirname.length()) {
			std::ostringstream str;
			str << "/home/" << getenv("USERNAME") << "/Documents/";
			dialog.set_current_folder(str.str().c_str());
		} else {
			dialog.set_current_folder(last_saved_dirname.c_str());
		}

		{
			std::ostringstream str;
			str << mw->get_plot().info().c_str() << ".png";
			dialog.set_current_name(str.str().c_str());
		}
		do {
			int rv = dialog.run();
			if (rv != GTK_RESPONSE_ACCEPT) return;

			do_extra = extra->resize->get_active();
			if (do_extra) {
				if (!(extra->x(newx)) || !(extra->y(newy))) {
					std::ostringstream str;
					str << "Output resolution must be numeric";
					Util::alert(&dialog, str.str());
					continue;
				}
				do_antialias = extra->antialias->get_active();
			}
			filename = dialog.get_filename();
		} while(0);
	}
	last_saved_dirname.clear();
	last_saved_dirname.append(filename);
	// Trim the filename to leave the dir:
	int spos = last_saved_dirname.rfind('/');
	if (spos >= 0)
		last_saved_dirname.erase(spos+1);


	if (do_extra) {
		SaveAsPNG *job = new SaveAsPNG();

		const Plot2& oldplot(mw->get_plot());
		const int aafactor = do_antialias ? 2 : 1;
		job->plot = new Plot2(mw->fractal, oldplot.centre, oldplot.size,
				newx * aafactor, newy * aafactor);
		job->plot->set_prefs(&mw->prefs());
		job->reporter = new PNGProgressWindow(mw, job);
		job->pal = mw->pal;
		job->antialias = aafactor;
		job->filename = filename;

		job->plot->start(job->reporter);
		// and commit it to the four winds.
	} else {
		mw->get_progbar()->set_text("Saving...");
		to_png(mw, mw->get_rwidth(), mw->get_rheight(), &mw->get_plot(),
				mw->pal, mw->get_antialias(), filename);
	}
}

SaveAsPNG::~SaveAsPNG()
{
	delete plot;
	delete reporter;
}
