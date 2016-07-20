/*
    MovieWindow: brot2 movie making control
    Copyright (C) 2016 Ross Younger

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

#include "png.h" // see launchpad 218409
#include "MovieWindow.h"
#include "MainWindow.h"
#include "Fractal.h"
#include "Prefs.h"
#include "Exception.h"
#include "misc.h"
#include "Plot3Plot.h"

#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/frame.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/enums.h>
#include <gtkmm/alignment.h>

#include <sstream>

using namespace BrotPrefs;

class MovieWindowPrivate {
	friend class MovieWindow;
	Util::HandyEntry<unsigned> *f_height, *f_width; // GTK::manage()
	Gtk::Label *next_here_label; // Managed

	MovieWindowPrivate() : f_height(0), f_width(0)
	{
	}
};

#define NewLabelX(_txt, _x1, _x2, _y1, _y2, _ref) do { \
	Gtk::Label *lbl = Gtk::manage(new Gtk::Label()); \
	lbl->set_markup(_txt); \
	this->inner->attach(*lbl, _x1, _x2, _y1, _y2); \
	_ref = lbl; \
} while(0)
#define NewLabel(_txt, _x1, _x2, _y1, _y2) do { Gtk::Label *_ptr; NewLabelX(_txt, _x1, _x2, _y1, _y2, _ptr); (void)_ptr;} while(0)

template<typename T>
Gtk::Label* NewLabelNum_(const T& val) {
	std::ostringstream tmp;
	tmp << val;
	Gtk::Label *lbl = Gtk::manage(new Gtk::Label(tmp.str().c_str()));
	return lbl;
}

#define NewLabelNum(_v, _x1, _x2, _y1, _y2) this->inner->attach(* NewLabelNum_(_v), _x1, _x2, _y1, _y2)

/* Updates inner table contents for the current set of prefs */
void MovieWindow::update_inner_table(bool initial) {
	this->inner->resize(movie.points.size()+2, 6);

	// Top row: Header
	// Data rows each contain: Centre, Size, Hold Frames, Frames to Next
	// Bottom row: (Next point goes here...)

	if (initial) {
		// TODO LATER Add tooltips?
		NewLabel("<b>Centre Re</b>", 0, 1, 0, 1);
		NewLabel("<b>Centre Im</b>", 1, 2, 0, 1);
		NewLabel("<b>Size Re</b>", 2, 3, 0, 1);
		NewLabel("<b>Size Im</b>", 3, 4, 0, 1);
		NewLabel("<b>Hold frames</b>", 4, 5, 0, 1);    // Settable
		NewLabel("<b>Frames to next</b>", 5, 6, 0, 1); // Settable
	}

	int row = 1;
	for (auto it = movie.points.begin(); it != movie.points.end(); it++) {
		NewLabelNum(it->centre.real(), 0, 1, row, row+1);
		NewLabelNum(it->centre.imag(), 1, 2, row, row+1);
		NewLabelNum(it->size.real(),   2, 3, row, row+1);
		NewLabelNum(it->size.imag(),   3, 4, row, row+1);
		NewLabelNum(it->hold_frames,   4, 5, row, row+1);
		NewLabelNum(it->frames_to_next,5, 6, row, row+1);
		++row;
	}
	// TODO: Make hold frames and frames-to-next settable (somehow)
	// TODO: Delete points by interacting with this table (somehow)

	if (initial) {
		NewLabelX("<i>Next point goes here...</i>", 0, 6, row, row+1, priv->next_here_label);
	} else {
		this->inner->remove(*priv->next_here_label);
		this->inner->attach(*priv->next_here_label, 0, 6, row, row+1);
	}
	++row;
	this->inner->show_all();
}

MovieWindow::MovieWindow(MainWindow& _mw, std::shared_ptr<const Prefs> prefs) : mw(_mw), _prefs(prefs)
{
	priv = new MovieWindowPrivate();
	set_title("Make movie");

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox());

	Gtk::Table *tbl;
	Gtk::Label *lbl;

	Gtk::Frame *wholemovie = Gtk::manage(new Gtk::Frame("Movie Options"));
    tbl = Gtk::manage(new Gtk::Table());
	lbl = Gtk::manage(new Gtk::Label("Height"));
	tbl->attach(*lbl, 0, 1, 0, 1);
	priv->f_height = Gtk::manage(new Util::HandyEntry<unsigned>(5));
	tbl->attach(*(priv->f_height), 1, 2, 0, 1);
	lbl = Gtk::manage(new Gtk::Label("Width"));
	tbl->attach(*lbl, 2, 3, 0, 1, Gtk::AttachOptions::FILL, Gtk::AttachOptions::FILL|Gtk::AttachOptions::EXPAND, 10);
	priv->f_width = Gtk::manage(new Util::HandyEntry<unsigned>(5));
	tbl->attach(*(priv->f_width), 3, 4, 0, 1);
	// TODO add the other whole-movie controls:
	//     fractal, palette
	//     Hud, AA
	//     FPS
	//     Output filename/etc.
	// TODO Set default height,width,others
	// TODO Default field width is a bit large for small ints
	wholemovie->add(*tbl);
	vbox->pack_start(*wholemovie);

	Gtk::Frame *keyframes = Gtk::manage(new Gtk::Frame("Key Frames"));
	this->inner = Gtk::manage(new Gtk::Table());
	this->inner->set_col_spacings(10);
	update_inner_table(true);
	keyframes->add(*inner);
	vbox->pack_start(*keyframes);

	Gtk::Button *btn;
    tbl = Gtk::manage(new Gtk::Table());
	tbl->set_col_spacings(10);
	btn = Gtk::manage(new Gtk::Button("Add current plot"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_add));
	tbl->attach(*btn, 0, 1, 0, 1);
	btn = Gtk::manage(new Gtk::Button("Reset"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_reset));
	tbl->attach(*btn, 1, 2, 0, 1);
	btn = Gtk::manage(new Gtk::Button("Render"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_render));
	tbl->attach(*btn, 2, 3, 0, 1);

	vbox->pack_end(*tbl);
	this->add(*vbox);
	hide();
	vbox->show_all();

	// TODO this window shouldn't appear over the main window if possible
}

MovieWindow::~MovieWindow() {
	delete priv;
}

void MovieWindow::do_add() {
	Plot3::Plot3Plot& plot = mw.get_plot();
	struct KeyFrame kf;
	kf.centre = plot.centre;
	kf.size = plot.size;
	kf.hold_frames = 0;
	kf.frames_to_next = 0;
	movie.points.push_back(kf);
	update_inner_table(false);
	// TODO Once we have delete, check for leaks
}
void MovieWindow::do_reset() {
	Util::alert(this, "Reset NYI");
	// TODO WRITEME
}
void MovieWindow::do_render() {
	Util::alert(this, "Render NYI");
	// TODO WRITEME
}

void MovieWindow::reset() {
	movie.fractal = 0;
	movie.palette = 0;
	movie.points.clear();
}

bool MovieWindow::close() {
	hide();
	return true;
}

bool MovieWindow::on_delete_event(GdkEventAny *evt) {
	(void)evt;
	return close();
}
