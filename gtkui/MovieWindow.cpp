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

namespace Fractal { typedef long double Value; }
// This definition exists to give a compile-time error if we change underlying maths and forget to fix up the Gtk TreeView code. See below (grep for glibmm__CustomBoxed_t).

class ModelColumns : public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() {
			add(m_centre_re); add(m_centre_im);
			add(m_size_re); add(m_size_im);
			add(m_hold_frames); add(m_frames_next);
		}
		Gtk::TreeModelColumn<Fractal::Value> m_centre_re;
		Gtk::TreeModelColumn<Fractal::Value> m_centre_im;
		Gtk::TreeModelColumn<Fractal::Value> m_size_re;
		Gtk::TreeModelColumn<Fractal::Value> m_size_im;
		Gtk::TreeModelColumn<unsigned> m_hold_frames;
		Gtk::TreeModelColumn<unsigned> m_frames_next;
};

class MovieWindowPrivate {
	friend class MovieWindow;
	Util::HandyEntry<unsigned> *f_height, *f_width; // GTK::manage()
	Gtk::Label *next_here_label; // Managed
	ModelColumns m_columns;
	Gtk::TreeView m_keyframes;
	Glib::RefPtr<Gtk::ListStore> m_refTreeModel;

	MovieWindowPrivate() : f_height(0), f_width(0)
	{
	}
};

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
	tbl->attach(*lbl, 0, 1, 0, 1, Gtk::AttachOptions::FILL, Gtk::AttachOptions::FILL|Gtk::AttachOptions::EXPAND, 5);
	priv->f_height = Gtk::manage(new Util::HandyEntry<unsigned>(5));
	tbl->attach(*(priv->f_height), 1, 2, 0, 1, Gtk::AttachOptions::SHRINK);
	lbl = Gtk::manage(new Gtk::Label("Width"));
	tbl->attach(*lbl, 2, 3, 0, 1, Gtk::AttachOptions::FILL, Gtk::AttachOptions::FILL|Gtk::AttachOptions::EXPAND, 5);
	priv->f_width = Gtk::manage(new Util::HandyEntry<unsigned>(5));
	tbl->attach(*(priv->f_width), 3, 4, 0, 1, Gtk::AttachOptions::SHRINK);

	// Defaults:
	priv->f_height->update(300);
	priv->f_width->update(300);
	// TODO add the other whole-movie controls:
	//     fractal, palette
	//     Hud, AA
	//     FPS
	//     Output filename/etc.
	// TODO Set default height,width,others
	wholemovie->add(*tbl);
	vbox->pack_start(*wholemovie);

	Gtk::Frame *keyframes = Gtk::manage(new Gtk::Frame("Key Frames"));
	priv->m_refTreeModel = Gtk::ListStore::create(priv->m_columns);
	priv->m_keyframes.set_model(priv->m_refTreeModel);
	keyframes->add(priv->m_keyframes);
	vbox->pack_start(*keyframes);

	// The GTK TreeView code can only automatically handle certain types and we are likely going to have to create a custom CellRenderer in what is currently ColumnFV here.
	// Grep for glibmm__CustomBoxed_t in /usr/include/gtkmm-2.4/gtkmm/treeview.h and read that comment carefully.
#define ColumnFV(_title, _field) do { priv->m_keyframes.append_column_numeric(_title, priv->m_columns._field, "%Lf"); } while(0)
#define ColumnEditable(_title, _field) do { priv->m_keyframes.append_column_editable(_title, priv->m_columns._field); } while(0)

	ColumnFV("Centre Real", m_centre_re);
	ColumnFV("Centre Imag", m_centre_im);
	ColumnFV("Size Real", m_size_re);
	ColumnFV("Size Imag", m_size_im);
	// TODO Would really like to make these columns distinct in some way:
	ColumnEditable("Hold Frames", m_hold_frames);
	ColumnEditable("Traverse Frames", m_frames_next);
	// LATER: Tooltips (doesn't seem possible to retrieve the actual widget of a standard column head with gtk 2.24?)
	// LATER: cell alignment?

	Gtk::Button *btn;
    tbl = Gtk::manage(new Gtk::Table());
	tbl->set_col_spacings(10);
	btn = Gtk::manage(new Gtk::Button("Add current plot"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_add));
	tbl->attach(*btn, 0, 1, 0, 1);
	btn = Gtk::manage(new Gtk::Button("Render"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_render));
	tbl->attach(*btn, 1, 2, 0, 1);
	btn = Gtk::manage(new Gtk::Button("Delete selected plot"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_delete));
	tbl->attach(*btn, 0, 1, 2, 3);
	btn = Gtk::manage(new Gtk::Button("Reset"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_reset));
	tbl->attach(*btn, 1, 2, 2, 3);

	vbox->pack_end(*tbl);
	this->add(*vbox);
	hide();
	vbox->show_all();

	// LATER this window shouldn't appear over the main window if possible
}

MovieWindow::~MovieWindow() {
	delete priv;
}

void MovieWindow::do_add() {
	Plot3::Plot3Plot& plot = mw.get_plot();

	Gtk::TreeModel::Row row = *(priv->m_refTreeModel->append());
	row[priv->m_columns.m_centre_re] = plot.centre.real();
	row[priv->m_columns.m_centre_im] = plot.centre.imag();
	row[priv->m_columns.m_size_re] = plot.size.real();
	row[priv->m_columns.m_size_im] = plot.size.imag();
	row[priv->m_columns.m_hold_frames] = 0;
	row[priv->m_columns.m_frames_next] = 100;
}
void MovieWindow::do_delete() {
	auto rp = priv->m_keyframes.get_selection()->get_selected();
	priv->m_refTreeModel->erase(*rp);
}
void MovieWindow::do_reset() {
	priv->m_refTreeModel->clear();
}
void MovieWindow::do_render() {
	movie.points.clear();
	auto rows = priv->m_refTreeModel->children();
	for (auto it = rows.begin(); it != rows.end(); it++) {
		struct KeyFrame kf;
		kf.centre.real((*it)[priv->m_columns.m_centre_re]);
		kf.centre.imag((*it)[priv->m_columns.m_centre_im]);
		kf.size.real((*it)[priv->m_columns.m_size_re]);
		kf.size.imag((*it)[priv->m_columns.m_size_im]);
		kf.hold_frames = (*it)[priv->m_columns.m_hold_frames];
		kf.frames_to_next = (*it)[priv->m_columns.m_frames_next];
		movie.points.push_back(kf);
	}

	if (!priv->f_height->read(movie.height)) {
		Util::alert(this, "Cannot parse height");
		priv->f_height->set_text("");
		priv->f_height->grab_focus();
		return;
	}
	if (!priv->f_width->read(movie.width)) {
		Util::alert(this, "Cannot parse width");
		priv->f_width->set_text("");
		priv->f_width->grab_focus();
		return;
	}
	// Temporary for now, just spit out the key frames to show we've read them correctly
	int id=0;
	for (auto it = movie.points.begin(); it != movie.points.end(); it++) {
		std::cout
			<< "KF " << id
			<< " C @ " << it->centre.real() << " + " << it->centre.imag() << "i;"
			<< " size " << it->size.real() << " + " << it->size.imag() << "i;"
			<< " hold " << it->hold_frames << ";"
			<< " intermediates " << it->frames_to_next
			<< std::endl;
		++id;
	}
	Util::alert(this, "Render NYI"); // TODO
}

void MovieWindow::reset() {
	movie.fractal = 0;
	movie.palette = 0;
	movie.points.clear();
}

bool MovieWindow::close() {
	hide();
	do_reset(); // So next time we open up we're fresh
	// LATER: If they haven't rendered, ask if they're sure.
	return true;
}

bool MovieWindow::on_delete_event(GdkEventAny *evt) {
	(void)evt;
	return close();
}
