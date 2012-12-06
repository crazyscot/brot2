/*
    SaveAsPNG: PNG export action/support for brot2
    Copyright (C) 2011-12 Ross Younger

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

#include "png.h" // must be first, see launchpad 218409

#include "SaveAsPNG.h"
#include "libbrot2/Render2.h"
#include "libbrot2/ChunkDivider.h"
#include "misc.h"

#include "MainWindow.h"
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/table.h>
#include <gtkmm/main.h>
#include <iostream>
#include <fstream>

using namespace std;
using namespace Plot3;
using namespace BrotPrefs;

std::string SaveAsPNG::last_saved_dirname = "";

void SaveAsPNG::to_png(MainWindow *mw, unsigned rwidth, unsigned rheight,
		Plot3Plot* plot, BasePalette* pal, bool antialias,
		std::string& filename)
{
	ofstream f(filename, ios::out | ios::trunc | ios::binary);
	if (f.is_open()) {
		Render2::PNG png(rwidth, rheight, *pal, -1, antialias);
		png.process(plot->get_chunks__only_after_completion());
		png.write(filename);
		if (f.bad()) {
			Util::alert(mw, "Writing failed");
		}
	} else {
		ostringstream str;
		str << "Could not open " << filename << " for writing.";
		Util::alert(mw, str.str());
	}
}

PNGProgressWindow::PNGProgressWindow(MainWindow& p, SaveAsPNG& j) : parent(p), job(j), _chunks_this_pass(0) {
	set_transient_for(parent);
	set_title("PNG export");
	Gtk::VBox* box = Gtk::manage(new Gtk::VBox());
	progbar = Gtk::manage(new Gtk::ProgressBar());
	box->pack_start(*progbar);
	progbar->set_text("Rendering pass 1...");

	add(*box);
	show_all();
}

void PNGProgressWindow::chunk_done(Plot3Chunk*) {
	// We're not doing anything with the completed chunks, they're picked up en masse at the end.
	_chunks_this_pass++;
	float workdone = (float) _chunks_this_pass / job.get_chunks_count();
	gdk_threads_enter();
	progbar->set_fraction(workdone);
	progbar->queue_draw();
	queue_draw();
	gdk_flush();
	gdk_threads_leave();
}

void PNGProgressWindow::pass_complete(std::string& commentary) {
	_chunks_this_pass=0;
	gdk_threads_enter();
	progbar->set_text(commentary);
	progbar->set_fraction(0.0);
	parent.queue_draw();
	gdk_flush();
	gdk_threads_leave();
}

void PNGProgressWindow::plot_complete() {
	std::shared_ptr<SaveAsPNG> png (&job);
	parent.queue_png(png);
}


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

/* STATIC */
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
		// HARD CASE: Launch a new job in the background to rerender.

		SaveAsPNG* job = new SaveAsPNG(mw, mw->get_plot(), newx, newy, do_antialias, filename);

		job->start();
		// and commit it to the four winds. Will be deleted later by mw...
	} else {
		// EASY CASE: Just save out of the current plot.
		mw->get_progbar()->set_text("Saving...");
		to_png(mw, mw->get_rwidth(), mw->get_rheight(), &mw->get_plot(),
				mw->pal, mw->is_antialias(), filename);
	}
}

SaveAsPNG::SaveAsPNG(MainWindow* mw, Plot3Plot& oldplot, unsigned width, unsigned height, bool antialias, string& name) :
		reporter(*mw,*this), divider(), aafactor(antialias ? 2 : 1),
		plot(mw->get_threadpool(), &reporter, *mw->fractal, divider, oldplot.centre, oldplot.size, width*aafactor, height*aafactor, 0),
		pal(mw->pal), filename(name)
{
	std::shared_ptr<const Prefs> pp = mw->prefs();
	plot.set_prefs(pp);
}

void SaveAsPNG::start(void)
{
	plot.start();
}

void SaveAsPNG::wait(void)
{
	plot.wait();
}

SaveAsPNG::~SaveAsPNG()
{
}
