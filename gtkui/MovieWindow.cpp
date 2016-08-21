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
#include "MovieProgress.h"
#include "MainWindow.h"
#include "Fractal.h"
#include "Prefs.h"
#include "Exception.h"
#include "misc.h"
#include "gtkutil.h"
#include "Plot3Plot.h"
#include "SaveAsPNG.h"
#include "gtkmain.h"

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
			add(m_hold_frames); add(m_speed_zoom); add(m_speed_translate);
		}
		Gtk::TreeModelColumn<Fractal::Value> m_centre_re;
		Gtk::TreeModelColumn<Fractal::Value> m_centre_im;
		Gtk::TreeModelColumn<Fractal::Value> m_size_re;
		Gtk::TreeModelColumn<Fractal::Value> m_size_im;
		Gtk::TreeModelColumn<unsigned> m_hold_frames;
		Gtk::TreeModelColumn<unsigned> m_speed_zoom;
		Gtk::TreeModelColumn<unsigned> m_speed_translate;
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

/* Cell data renderer for variable-precision floating-point data.
 * We need to grobble around in the MovieWindowPrivate to determine whether
 * this field is real or imaginary, and thence which fields to grab the sizing
 * information for Fractal::precision_for().
 *
 * Based on gtkmm/treeview.h _auto_cell_data_func.
 */
void MovieWindow::treeview_fractal_cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter, int model_column) {
	Gtk::CellRendererText* pTextRenderer = dynamic_cast<Gtk::CellRendererText*>(cell);
	if(!pTextRenderer) {
		g_warning("treeview_append_fractal_column was used with a non-text field.");
		return;
	}
	if (iter) {
		//Get the value from the model.
		Gtk::TreeModel::Row row = *iter;
		Fractal::Value value;
		row.get_value(model_column, value);

		// Am I real or imaginary?
		Fractal::Value fieldsize;
		unsigned npixels;

		if (    (model_column == priv->m_columns.m_centre_re.index()) ||
				(model_column == priv->m_columns.m_size_re.index()) ) {
			// This column is a real component
			row.get_value( priv->m_columns.m_size_re.index(), fieldsize );
			if (!priv->f_width.read(npixels))
				npixels = 300; // horrid default fallback if field is unparseable
		} else if (
				(model_column == priv->m_columns.m_centre_im.index()) ||
				(model_column == priv->m_columns.m_size_im.index()) ) {
			// This column is an imaginary component
			row.get_value( priv->m_columns.m_size_im.index(), fieldsize );
			if (!priv->f_height.read(npixels))
				npixels = 300; // horrid default fallback if field is unparseable
		} else {
			g_warning("treeview_append_fractal_column was used with a new field, couldn't determine re/im");
			return;
		}
		std::ostringstream buf;
		buf << std::setprecision(Fractal::precision_for(fieldsize, npixels)) << value;
		pTextRenderer->property_text() = buf.str().c_str();
	}
}

/* Based on gtkmm/treeview.h TreeView::append_column_numeric */
int MovieWindow::treeview_append_fractal_column(Gtk::TreeView& it, const Glib::ustring& title, const Gtk::TreeModelColumn<Fractal::Value>& model_column) {
	Gtk::CellRenderer* pCellRenderer = manage( new Gtk::CellRendererText() );
	Gtk::TreeViewColumn* const pViewColumn = Gtk::manage( new Gtk::TreeViewColumn(title, *pCellRenderer) );
	//typedef void (*type_fptr)(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter, int model_column, const Glib::ustring& format);
	//type_fptr fptr = treeview_fractal_cell_data_func;
	Gtk::TreeViewColumn::SlotCellData slot = sigc::bind<-1>(
			sigc::mem_fun(this, &MovieWindow::treeview_fractal_cell_data_func),
			model_column.index()
			);
	pViewColumn->set_cell_data_func(*pCellRenderer, slot);
	return it.append_column(*pViewColumn);
}

void MovieWindow::treeview_speed_cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter, int model_column) {
	Gtk::CellRendererText* pTextRenderer = dynamic_cast<Gtk::CellRendererText*>(cell);
	if(!pTextRenderer) {
		g_warning("treeview_append_speed_column was used with a non-text field.");
		return;
	}
	if (iter) {
		//Get the value from the model.
		Gtk::TreeModel::Row row = *iter;
		unsigned value;
		row.get_value(model_column, value);

		{ // Clamp if necessary
			unsigned newvalue = value;
			if (value<1) newvalue = 1;
			if (value>50) newvalue = 50;
			if (value != newvalue) {
				row.set_value(model_column, newvalue);
				value = newvalue;
			}
		}

		char buf[5];
		snprintf(buf, sizeof buf, "%u", value);
		pTextRenderer->property_text() = buf;
	}
}

int MovieWindow::treeview_append_speed_column(Gtk::TreeView& it, const Glib::ustring& title, const Gtk::TreeModelColumn<unsigned>& model_column) {
	Gtk::TreeViewColumn* const pViewColumn = Gtk::manage( new Gtk::TreeViewColumn(title) );
	Gtk::CellRendererText* pCellRenderer = manage( new Gtk::CellRendererText() );
	pViewColumn->pack_start(*pCellRenderer);

	Gtk::TreeViewColumn::SlotCellData slot = sigc::bind<-1>(
			sigc::mem_fun(this, &MovieWindow::treeview_speed_cell_data_func),
			model_column.index()
			);
	pViewColumn->set_cell_data_func(*pCellRenderer, slot);
	int rv = it.append_column(*pViewColumn);
	Gtk::TreeView_Private::_connect_auto_store_editable_signal_handler<unsigned>(&it, pCellRenderer, model_column);

	static Gdk::Color* cellbg = 0;
	if (!cellbg) {
		cellbg = new Gdk::Color();
		cellbg->set_rgb(65535,65535,25344);
	}
	pCellRenderer->property_background_gdk() = *cellbg;
	return rv;
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

	priv->f_height.signal_changed().connect(sigc::mem_fun(*this, &MovieWindow::do_update_duration)); // Must do this after setting initial value
	priv->f_width.signal_changed().connect(sigc::mem_fun(*this, &MovieWindow::do_update_duration)); // Must do this after setting initial value
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
#define ColumnFV(_title, _field) do { priv->m_keyframes.append_column_numeric(_title, priv->m_columns._field, "%.5Le"); } while(0)
#define ColumnFVVP(_title, _field) do { treeview_append_fractal_column(priv->m_keyframes, _title, priv->m_columns._field); } while(0)
#define ColumnEditable(_title, _field) do { priv->m_keyframes.append_column_editable(_title, priv->m_columns._field); } while(0)
#define ColumnSpeed(_title, _field) do { treeview_append_speed_column(priv->m_keyframes, _title, priv->m_columns._field); } while(0)

	ColumnFVVP("Centre Real", m_centre_re);
	ColumnFVVP("Centre Imag", m_centre_im);
	ColumnFV("Size Real", m_size_re);
	ColumnFV("Size Imag", m_size_im);
	ColumnEditable("Hold Frames", m_hold_frames);
	ColumnSpeed("Zoom Speed", m_speed_zoom);
	ColumnSpeed("Move Speed", m_speed_translate);
	// LATER: Tooltips (doesn't seem possible to retrieve the actual widget of a standard column head with gtk 2.24?)
	// LATER: cell alignment?

	Gtk::HButtonBox *bbox = Gtk::manage(new Gtk::HButtonBox());
	Gtk::Button *btn;
	btn = Gtk::manage(new Gtk::Button("Add current plot"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_add));
	bbox->pack_start(*btn);
	btn = Gtk::manage(new Gtk::Button("Render"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_render));
	bbox->pack_start(*btn);
	vbox->pack_start(*bbox);

	// Second row of buttons
	bbox = Gtk::manage(new Gtk::HButtonBox());
	btn = Gtk::manage(new Gtk::Button("Delete selected plot"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_delete));
	bbox->pack_start(*btn);
	btn = Gtk::manage(new Gtk::Button("Reset"));
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MovieWindow::do_reset));
	bbox->pack_start(*btn);

	vbox->pack_end(*bbox);
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
	row[priv->m_columns.m_speed_zoom] = 10;
	row[priv->m_columns.m_speed_translate] = 2;
	Gtk::TreePath path(row);

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
		Util::alert(this, "Fractal or palette have changed, only the last specified will be used to make the movie", Gtk::MessageType::MESSAGE_WARNING);
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

bool MovieWindow::update_movie_struct() {
	std::unique_lock<std::mutex> lock(mux);
	bool ok = true;
	auto rows = priv->m_refTreeModel->children();

	movie.points.clear();
	for (auto it = rows.begin(); it != rows.end(); it++) {
		struct Movie::KeyFrame kf(
				(*it)[priv->m_columns.m_centre_re], (*it)[priv->m_columns.m_centre_im],
				(*it)[priv->m_columns.m_size_re], (*it)[priv->m_columns.m_size_im],
				(*it)[priv->m_columns.m_hold_frames],
				(*it)[priv->m_columns.m_speed_zoom], (*it)[priv->m_columns.m_speed_translate]);
		movie.points.push_back(kf);
	}

	if (!priv->f_height.read(movie.height) || movie.height<1) {
		priv->f_height.set_error();
		ok = false;
	} else {
		priv->f_height.clear_error();
	}
	if (!priv->f_width.read(movie.width) || movie.width<1) {
		priv->f_width.set_error();
		ok = false;
	} else {
		priv->f_width.clear_error();
	}
	if (!priv->f_fps.read(movie.fps) || movie.fps<1) {
		priv->f_fps.set_error();
		ok = false;
	} else {
		priv->f_fps.clear_error();
	}
	movie.draw_hud = priv->f_hud.get_active();
	movie.antialias = priv->f_antialias.get_active();
	return ok;
}

void MovieWindow::do_render() {
	std::unique_lock<std::mutex> lock(mux);
	if (renderer) {
		Util::alert(this, "A movie is already rendering, please wait for it to finish");
		return;
	}
	// Doesn't make sense to make a movie with fewer than two points...
	auto rows = priv->m_refTreeModel->children();
	if (rows.size() < 2) {
		Util::alert(this, "You need to specify at least two key frames to make a movie");
		return;
	}
	lock.unlock();
	if (!update_movie_struct()) return;
	lock.lock();

	{
		bool ok = true;
		for (auto iter = movie.points.begin(); iter != movie.points.end(); iter++) {
			ok &= (real((*iter).size) != 0);
			ok &= (imag((*iter).size) != 0);
		}
		if (!ok) {
			Util::alert(this, "Frames with size 0 are not meaningful");
			return;
		}
	}

	{
		bool ok = true;
		for (auto iter = movie.points.begin(); iter != movie.points.end(); iter++) {
			ok &= ((*iter).speed_zoom != 0);
			ok &= ((*iter).speed_translate != 0);
		}
		if (!ok) {
			// Should never happen now we clamp the cell contents
			Util::alert(this, "All speeds must be non-0");
			return;
		}
	}

	std::string filename;
	if (!run_filename(filename, renderer))
		return;

	Movie::Progress* reporter = new Movie::Progress(movie, *renderer);
	renderer->start(*reporter, *this, filename, movie, mw.prefs(), mw.get_threadpool(), brot2_argv0);
	// reporter will be deleted when completion signalled
}

bool MovieWindow::run_filename(std::string& filename, std::shared_ptr<Movie::Renderer>& renrv)
{
	Gtk::FileChooserDialog dialog(*this, "Save Movie", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
	dialog.set_do_overwrite_confirmation(true);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::ResponseType::RESPONSE_ACCEPT);

	auto types = Movie::RendererFactory::all_factory_names();
	for (auto it=types.begin(); it!=types.end(); it++) {
		Gtk::FileFilter *filter = Gtk::manage(new Gtk::FileFilter());
		Movie::RendererFactory *ren = Movie::RendererFactory::get_factory(*it);
		filter->set_name(ren->name);
		filter->add_pattern(ren->pattern);
		dialog.add_filter(*filter);
	}
	dialog.set_current_folder(SavePNG::Base::default_save_dir());

	int rv = dialog.run();
	if (rv != Gtk::ResponseType::RESPONSE_ACCEPT) return false;
	Gtk::FileFilter *filter = dialog.get_filter();
	renrv = Movie::RendererFactory::get_factory(filter->get_name())->instantiate();
	filename = dialog.get_filename();
	SavePNG::Base::update_save_dir(filename);
	{
		// Attempt to enforce file extension.. there are probably better ways to do this.
		std::string extn(renrv->pattern);
		if (extn[0] == '*')
			extn.erase(0,1);
		if (!Util::ends_with(filename,extn))
			filename.append(extn);
	}
	return true;
}

bool MovieWindow::on_delete_event(GdkEventAny *) {
	std::unique_lock<std::mutex> lock(mux);
	if (renderer) {
		Gtk::MessageDialog dialog(*this, "Cancel this movie render?", false, Gtk::MessageType::MESSAGE_WARNING, Gtk::ButtonsType::BUTTONS_YES_NO, true);
		int response = dialog.run();
		if (response == Gtk::ResponseType::RESPONSE_NO)
			return true;
		renderer->request_cancel();
	}
	hide();
	lock.unlock();
	do_reset(); // So next time we open up we're fresh
	// LATER: If they haven't rendered, ask if they're sure.
	return false;
}

void MovieWindow::stop() {
	std::unique_lock<std::mutex> lock(mux);
	if (renderer) {
		renderer->request_cancel();
	}
}

void MovieWindow::wait() {
	std::unique_lock<std::mutex> lock(mux);
	if (renderer) {
		completion_cv.wait(lock);
	}
}


// Checks for errors in the fields and updates duration
void MovieWindow::do_update_duration() {
	if (!update_movie_struct()) {
		priv->f_duration.set_text("?");
		return;
	}
	std::unique_lock<std::mutex> lock(mux);
	unsigned frames = movie.count_frames();

	std::ostringstream msg;
	if (movie.fps > 0) {
		unsigned hr, min, sec, frm;
		frm = frames;
		sec = frm / movie.fps; frm %= movie.fps;
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
void MovieWindow::signal_completion(Movie::RenderJob& job) {
	std::unique_lock<std::mutex> lock(mux);
	ASSERT(&job._renderer == renderer.get());
	gdk_threads_enter();
	delete job._reporter; // We created it, we delete it. It's a Movie::Progress*, which is a Gtk::Window so we have to lock for it.
	gdk_threads_leave();
	renderer = 0;
	completion_cv.notify_all();
}
