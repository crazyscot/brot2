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
#include "MovieRender.h"
#include "MainWindow.h"
#include "Fractal.h"
#include "Prefs.h"
#include "Exception.h"
#include "misc.h"
#include "Plot3Plot.h"
#include "SaveAsPNG.h"

#include <gtkmm/dialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/table.h>
#include <gtkmm/frame.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/liststore.h>
#include <gtkmm/enums.h>
#include <gtkmm/alignment.h>

#include <iostream>
#include <iomanip>
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
	ModelColumns m_columns;
	Gtk::TreeView m_keyframes;
	Glib::RefPtr<Gtk::ListStore> m_refTreeModel;
	Util::HandyEntry<unsigned> f_height, f_width, f_fps;
	Gtk::CheckButton f_hud, f_antialias;
	Gtk::Entry f_duration;
	Gtk::Label f_fractal, f_palette;

	MovieWindowPrivate() : f_height(5), f_width(5), f_fps(5), f_hud("Draw HUD"), f_antialias("Antialias")
	{
		f_duration.set_editable(false);
		f_fractal.set_alignment(Gtk::ALIGN_START);
		f_palette.set_alignment(Gtk::ALIGN_START);
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

	tbl->attach(* Gtk::manage(new Gtk::Label("Fractal:")), 0, 1, 0, 1);
	tbl->attach(priv->f_fractal, 1, 4, 0, 1);
	tbl->attach(* Gtk::manage(new Gtk::Label("Palette:")), 4, 5, 0, 1);
	tbl->attach(priv->f_palette, 5, 7, 0, 1);
	priv->f_fractal.set_markup("<i>will appear here</i>");
	priv->f_palette.set_markup("<i>will appear here</i>");

	lbl = Gtk::manage(new Gtk::Label("Height"));
	tbl->attach(*lbl, 0, 1, 1, 2, Gtk::AttachOptions::FILL, Gtk::AttachOptions::FILL|Gtk::AttachOptions::EXPAND, 5);
	tbl->attach(priv->f_height, 1, 2, 1, 2, Gtk::AttachOptions::SHRINK);
	lbl = Gtk::manage(new Gtk::Label("Width"));
	tbl->attach(*lbl, 2, 3, 1, 2, Gtk::AttachOptions::FILL, Gtk::AttachOptions::FILL|Gtk::AttachOptions::EXPAND, 5);
	tbl->attach(priv->f_width, 3, 4, 1, 2, Gtk::AttachOptions::SHRINK);
	tbl->attach(priv->f_hud, 4,5, 1, 2);
	tbl->attach(priv->f_antialias, 5,6, 1, 2);
	tbl->attach(* Gtk::manage(new Gtk::Label("Frames per second")), 0, 3, 2, 3);
	tbl->attach(priv->f_fps, 3, 4, 2, 3, Gtk::AttachOptions::SHRINK);
	tbl->attach(* Gtk::manage(new Gtk::Label("Duration")), 4, 5, 2, 3);
	tbl->attach(priv->f_duration, 5, 6, 2, 3);

	// Defaults.
	// LATER: Could remember these from last time?
	priv->f_height.update(300);
	priv->f_width.update(300);
	priv->f_fps.update(25);
	priv->f_antialias.set_active(true);
	priv->f_hud.set_active(false);

	priv->f_fps.signal_changed().connect(sigc::mem_fun(*this, &MovieWindow::do_update_duration)); // Must do this after setting initial value

	wholemovie->add(*tbl);
	vbox->pack_start(*wholemovie);

	Gtk::Frame *keyframes = Gtk::manage(new Gtk::Frame("Key Frames"));
	priv->m_refTreeModel = Gtk::ListStore::create(priv->m_columns);
	priv->m_keyframes.set_model(priv->m_refTreeModel);
	priv->m_refTreeModel->signal_row_changed().connect(sigc::mem_fun(*this, &MovieWindow::do_update_duration2));
	keyframes->add(priv->m_keyframes);
	vbox->pack_start(*keyframes);

	// The GTK TreeView code can only automatically handle certain types and we are likely going to have to create a custom CellRenderer in what is currently ColumnFV here.
	// Grep for glibmm__CustomBoxed_t in /usr/include/gtkmm-2.4/gtkmm/treeview.h and read that comment carefully.
	// possible C example: http://scentric.net/tutorial/sec-treeview-col-celldatafunc.html
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
	tbl->attach(*btn, 0, 2, 0, 1);
	btn = Gtk::manage(new Gtk::Button("Render"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_render));
	tbl->attach(*btn, 2, 3, 0, 1);
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
	Gtk::TreePath path(row);
	priv->m_keyframes.set_cursor(path, *priv->m_keyframes.get_column(4), true); // !! Hard-wired column number

	bool changed=false;
	if (movie.fractal != &plot.fract) {
		std::ostringstream str;
		str << "<b>" << plot.fract.name << "</b>";
		priv->f_fractal.set_markup(str.str());
		if (movie.fractal)
			changed = true;
		movie.fractal = &plot.fract;
	}
	if (movie.palette != mw.pal) {
		std::ostringstream str;
		str << "<b>" << mw.pal->name << "</b>";
		priv->f_palette.set_markup(str.str());
		if (movie.palette)
			changed = true;
		movie.palette = mw.pal;
	}
	if (changed)
		Util::alert(this, "Fractal or palette have changed, only the last specified will be used to make the movie");
}
void MovieWindow::do_delete() {
	auto rp = priv->m_keyframes.get_selection()->get_selected();
	priv->m_refTreeModel->erase(*rp);
	do_update_duration();
}
void MovieWindow::do_reset() {
	priv->m_refTreeModel->clear();
	do_update_duration();
}
void MovieWindow::do_render() {
	// Doesn't make sense to make a movie with fewer than two points...
	auto rows = priv->m_refTreeModel->children();
	if (rows.size() < 2) {
		Util::alert(this, "You need to specify at least two key frames to make a movie");
		return;
	}

	movie.points.clear();
	for (auto it = rows.begin(); it != rows.end(); it++) {
		struct Movie::KeyFrame kf;
		kf.centre.real((*it)[priv->m_columns.m_centre_re]);
		kf.centre.imag((*it)[priv->m_columns.m_centre_im]);
		kf.size.real((*it)[priv->m_columns.m_size_re]);
		kf.size.imag((*it)[priv->m_columns.m_size_im]);
		kf.hold_frames = (*it)[priv->m_columns.m_hold_frames];
		kf.frames_to_next = (*it)[priv->m_columns.m_frames_next];
		movie.points.push_back(kf);
	}

	if (!priv->f_height.read(movie.height)) {
		Util::alert(this, "Cannot parse height");
		priv->f_height.set_text("");
		priv->f_height.grab_focus();
		return;
	}
	if (!priv->f_width.read(movie.width)) {
		Util::alert(this, "Cannot parse width");
		priv->f_width.set_text("");
		priv->f_width.grab_focus();
		return;
	}
	if (!priv->f_fps.read(movie.fps)) {
		Util::alert(this, "Cannot parse FPS");
		priv->f_fps.set_text("");
		priv->f_fps.grab_focus();
		return;
	}
	movie.draw_hud = priv->f_hud.get_active();
	movie.antialias = priv->f_antialias.get_active();

	std::string filename;
	Movie::Renderer * ren;
	if (!run_filename(filename, ren))
		return;

	ren->render(filename, movie);
}

inline bool ends_with(std::string const & value, std::string const & ending)
{
	/* Found on Stack Overflow */
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool MovieWindow::run_filename(std::string& filename, Movie::Renderer*& ren)
{
	Gtk::FileChooserDialog dialog(*this, "Save Movie", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
	dialog.set_do_overwrite_confirmation(true);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::ResponseType::RESPONSE_ACCEPT);

	auto types = Movie::Renderer::all_renderers.names();
	for (auto it=types.begin(); it!=types.end(); it++) {
		Gtk::FileFilter *filter = Gtk::manage(new Gtk::FileFilter());
		Movie::Renderer *ren = Movie::Renderer::all_renderers.get(*it);
		filter->set_name(ren->name);
		filter->add_pattern(ren->pattern);
		dialog.add_filter(*filter);
	}
	dialog.set_current_folder(SaveAsPNG::default_save_dir());

	int rv = dialog.run();
	if (rv != Gtk::ResponseType::RESPONSE_ACCEPT) return false;
	Gtk::FileFilter *filter = dialog.get_filter();
	ren = Movie::Renderer::all_renderers.get(filter->get_name());
	filename = dialog.get_filename();
	SaveAsPNG::update_save_dir(filename);
	{
		// Attempt to enforce file extension.. there are probably better ways to do this.
		std::string extn(ren->pattern);
		if (extn[0] == '*')
			extn.erase(0,1);
		if (!ends_with(filename,extn))
			filename.append(extn);
	}
	return true;
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

void MovieWindow::do_update_duration() {
	auto rows = priv->m_refTreeModel->children();
	unsigned frames = 0, last_traverse = 0;
	for (auto it = rows.begin(); it != rows.end(); it++) {
		frames += (*it)[priv->m_columns.m_hold_frames];
		// Don't count Traverse frames unless there is something to traverse to
		frames += last_traverse;
		last_traverse = (*it)[priv->m_columns.m_frames_next];
	}
	std::ostringstream msg;
	unsigned fps = 0;
	if (priv->f_fps.read(fps) && fps > 0) {
		unsigned hr, min, sec, frm;
		frm = frames;
		sec = frm / fps; frm %= fps;
		min = sec / 60;  sec %= 60;
		hr  = min / 60;  min %= 60;
		msg << std::setfill('0')
			<< std::setw(2) << hr << ":" << std::setw(2) << min << ":" << std::setw(2) << sec << ":" << std::setw(2) << frm;
	} else {
		msg << frames << " frames";
	}

	priv->f_duration.set_text(msg.str());
}
void MovieWindow::do_update_duration2(const Gtk::TreeModel::Path&, const Gtk::TreeModel::iterator&) {
	// For now this is just a thin wrapper. Might become something more complex later.
	do_update_duration();
}
