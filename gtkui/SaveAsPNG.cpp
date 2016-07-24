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
#include "BaseHUD.h"

#include "MainWindow.h"
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/table.h>
#include <gtkmm/main.h>
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace std;
using namespace Plot3;
using namespace BrotPrefs;
using namespace SavePNG;

void Single::instance_to_png(MainWindow *mw)
{
	Single::to_png(mw, _width, _height, &plot, pal, _do_antialias, _do_hud, filename);
}

void SavePNG::Base::save_png(Gtk::Window *parent)
{
	ofstream f(filename, ios::out | ios::trunc | ios::binary);
	if (f.is_open()) {
		Render2::PNG png(_width, _height, *pal, -1, _do_antialias);
		png.process(plot.get_chunks__only_after_completion());
		if (_do_hud)
			BaseHUD::apply(png, prefs, &plot, false, false);
		png.write(filename);
		if (f.bad()) {
			Util::alert(parent, "Writing failed");
		}
	} else {
		ostringstream str;
		str << "Could not open " << filename << " for writing.";
		Util::alert(parent, str.str());
	}
}

/*STATIC*/
// XXX Factor this out
void Single::to_png(Gtk::Window *parent, unsigned rwidth, unsigned rheight,
		Plot3Plot* plot, const BasePalette* pal, bool antialias, bool show_hud,
		std::string& filename)
{
    std::shared_ptr<const Prefs> prefs = Prefs::getMaster();
	ofstream f(filename, ios::out | ios::trunc | ios::binary);
	if (f.is_open()) {
		Render2::PNG png(rwidth, rheight, *pal, -1, antialias);
		png.process(plot->get_chunks__only_after_completion());
		if (show_hud) {
			BaseHUD::apply(png, prefs, plot, false, false);
		}
		png.write(filename);
		if (f.bad()) {
			Util::alert(parent, "Writing failed");
		}
	} else {
		ostringstream str;
		str << "Could not open " << filename << " for writing.";
		Util::alert(parent, str.str());
	}
}

SingleProgressWindow::SingleProgressWindow(MainWindow& p, Single& j) : parent(p), job(j), _chunks_this_pass(0) {
	set_transient_for(parent);
	set_title("Save as PNG");
	Gtk::VBox* box = Gtk::manage(new Gtk::VBox());
	progbar = Gtk::manage(new Gtk::ProgressBar());
	box->pack_start(*progbar);
	progbar->set_text("Rendering pass 1...");

	add(*box);
	show_all();
}

void SingleProgressWindow::chunk_done(Plot3Chunk*) {
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

void SingleProgressWindow::pass_complete(std::string& commentary) {
	_chunks_this_pass=0;
	gdk_threads_enter();
	progbar->set_text(commentary);
	progbar->set_fraction(0.0);
	parent.queue_draw();
	gdk_flush();
	gdk_threads_leave();
}

void SingleProgressWindow::plot_complete() {
	std::shared_ptr<Single> png (&job);
	parent.queue_png(png);
}


class FileChooserExtra : public Gtk::VBox {
	public:
		Gtk::CheckButton *resize, *antialias, *do_hud;
		Gtk::HBox *inner;
		Util::HandyEntry<int> *f_x, *f_y;

		FileChooserExtra(int init_x, int init_y) {
			Gtk::Label *lbl;

			resize = Gtk::manage(new Gtk::CheckButton("Save options (requires re-render)"));
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

			do_hud = Gtk::manage(new Gtk::CheckButton("Render HUD"));
			do_hud->set_active(false);
			inner->pack_start(*do_hud);

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
void Single::do_save(MainWindow *mw)
{
	std::string filename;
	int newx=0, newy=0;
	bool do_extra = false, do_antialias = false, do_hud = false;
    std::shared_ptr<const Prefs> prefs = Prefs::getMaster();

	if(mw->get_plot().is_running()) {
        std::ostringstream str;
        str << "Cannot save while a plot is working";
        Util::alert(mw, str.str());
        return;
    }
	{
		Gtk::FileChooserDialog dialog(*mw, "Save File", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
		dialog.set_do_overwrite_confirmation(true);
		dialog.add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
		dialog.add_button(Gtk::Stock::SAVE, Gtk::ResponseType::RESPONSE_ACCEPT);

		FileChooserExtra *extra = Gtk::manage(new FileChooserExtra(
					mw->get_rwidth(), mw->get_rheight() ));
		dialog.set_extra_widget(*extra);

		dialog.set_current_folder(default_save_dir());
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
				do_hud = extra->do_hud->get_active();
			}
			filename = dialog.get_filename();
		} while(0);
	}
	update_save_dir(filename);

	if (do_extra) {
		// HARD CASE: Launch a new job in the background to rerender.

		/* LP#1600574 Aspect ratio fix */
		double aspect = (double) newx / (double) newy;
		Plot3::Plot3Plot& plot = mw->get_plot();
		Fractal::Point centre = plot.centre,
			size = plot.size;
		if (imag(size) * aspect != real(size))
			size.imag(real(size) / aspect);

		Single* job = new Single(mw, centre, size, newx, newy, do_antialias, do_hud, filename);

		job->start();
		// and commit it to the four winds. Will be deleted later by mw...
	} else {
		// EASY CASE: Just save out of the current plot.
		mw->get_progbar()->set_text("Saving...");
		to_png(mw, mw->get_rwidth(), mw->get_rheight(), &mw->get_plot(),
				mw->pal, mw->is_antialias(), false, filename);
		mw->get_progbar()->set_text("Save complete");
	}
}

SavePNG::Base::Base(std::shared_ptr<const Prefs> _prefs, ThreadPool& threads, const Fractal::FractalImpl& fractal, const BasePalette& palette, Plot3::IPlot3DataSink& sink, Fractal::Point centre, Fractal::Point size, unsigned width, unsigned height, bool antialias, bool do_hud, string& fname) :
		prefs(_prefs),
		divider(new Plot3::ChunkDivider::Horizontal10px()), aafactor(antialias ? 2 : 1),
		plot(threads, &sink, fractal, *divider, centre, size, width*aafactor, height*aafactor, 0),
		pal(&palette), filename(fname), _width(width), _height(height), _do_antialias(antialias), _do_hud(do_hud)
{
	plot.set_prefs(_prefs);
}

Single::Single(MainWindow* mw, Fractal::Point centre, Fractal::Point size, unsigned width, unsigned height, bool antialias, bool do_hud, string& filename) :
		Base(mw->prefs(), mw->get_threadpool(), *mw->fractal, *mw->pal, reporter, centre, size, width, height, antialias, do_hud, filename),
		reporter(*mw, *this)
{
}

MovieFrame::MovieFrame(std::shared_ptr<const Prefs> prefs, ThreadPool& threads, const Fractal::FractalImpl& fractal, const BasePalette& palette, Plot3::IPlot3DataSink& sink, Fractal::Point centre, Fractal::Point size, unsigned width, unsigned height, bool antialias, bool do_hud, string& filename) :
		Base(prefs, threads, fractal, palette, sink, centre, size, width, height, antialias, do_hud, filename)
{
}


void SavePNG::Base::start(void)
{
	plot.start();
}

void SavePNG::Base::wait(void)
{
	plot.wait();
}

SavePNG::Base::~Base()
{
}

SavePNG::Single::~Single()
{
}

SavePNG::MovieFrame::~MovieFrame()
{
}


std::string SavePNG::Base::last_saved_dirname = "";

/*STATIC*/
std::string SavePNG::Base::default_save_dir()
{
	if (!last_saved_dirname.length()) {
		std::shared_ptr<const Prefs> prefs = Prefs::getMaster();
		last_saved_dirname = prefs->get(PREF(LastSaveDir));
	}
	if (!last_saved_dirname.length()) {
		const char *homedir;
		if ((homedir = getenv("HOME")) == NULL) {
			homedir = getpwuid(getuid())->pw_dir;
		}
		// Try $HOME/Documents
		std::ostringstream str;
		str << homedir << "/Documents/";
		if (Glib::file_test(str.str(), Glib::FILE_TEST_IS_DIR)) {
			// use str as-is
		} else {
			str.flush();
			str << homedir;
		}
		return str.str();
	} else {
		return last_saved_dirname;
	}
}

/*STATIC*/
void SavePNG::Base::update_save_dir(const std::string& filename)
{
	last_saved_dirname.clear();
	last_saved_dirname.append(filename);
	// Trim the filename to leave the dir:
	int spos = last_saved_dirname.rfind('/');
	if (spos >= 0)
		last_saved_dirname.erase(spos+1);
    {
		std::shared_ptr<const Prefs> prefs = Prefs::getMaster();
        std::shared_ptr<Prefs> prefs2 = prefs->getWorkingCopy();
        prefs2->set(PREF(LastSaveDir), last_saved_dirname);
        prefs2->commit();
    }
}
