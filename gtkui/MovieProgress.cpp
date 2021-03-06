/*
    MovieProgress: brot2 movie progress window
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

#include "MovieProgress.h"
#include "MovieMode.h"
#include "MovieRender.h"
#include "MovieWindow.h"
#include "gtkutil.h"
#include <sstream>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/messagedialog.h>

Movie::Progress::Progress(MovieWindow& _parent, const struct MovieInfo &_movie, Movie::Renderer& _ren) : parent(_parent), renderer(_ren),
	chunks_done(0), chunks_count(0), frames_done(-1),
	npixels(_movie.height * _movie.width * (_movie.antialias ? 4 : 1)),
	nframes(_movie.count_frames()), movie(_movie)
{
	// Precondition: GDK threads lock held
	vbox = Gtk::manage(new Gtk::VBox());
	plotbar = Gtk::manage(new Gtk::ProgressBar());
	framebar = Gtk::manage(new Gtk::ProgressBar());
	moviebar = Gtk::manage(new Gtk::ProgressBar());
	set_title("Movie progress");
	set_type_hint(Gdk::WindowTypeHint::WINDOW_TYPE_HINT_UTILITY);

	Gtk::Label *lbl;
#define LABEL(_txt) do { lbl = Gtk::manage(new Gtk::Label(_txt)); vbox->pack_start(*lbl); } while(0)
	LABEL("Current pass");
	vbox->pack_start(*plotbar);
	LABEL("Current plot");
	vbox->pack_start(*framebar);
	LABEL("Overall");
	vbox->pack_start(*moviebar);

	Gtk::HButtonBox *bbox = Gtk::manage(new Gtk::HButtonBox());
	vbox->pack_end(*bbox);

	cancel_btn = Gtk::manage(new Gtk::Button("Cancel"));
	cancel_btn->signal_clicked().connect(sigc::mem_fun(*this, &Movie::Progress::do_cancel));
	bbox->pack_end(*cancel_btn);

	add(*vbox);
	show_all();
	Glib::signal_timeout().connect( sigc::mem_fun(*this, &Movie::Progress::on_timer), 300 );
	// Cannot call plot_complete() here as we already hold the GDK lock
	frames_traversed_gdklocked(1);

	// And set up an initial position.. Try to put this left of the Movie window.
	// This is tricky as the window resizes itself around its content soon after construction.
	// For now I'm going to cheat horribly and hard-wire what it ends up as on my machine :-)
#define ASSUMED_PROGRESS_WIDTH 280
	int ww, hh, px, py;
	Util::get_screen_geometry(*this, ww, hh);
	parent.get_position(px, py);
	int xx = px - ASSUMED_PROGRESS_WIDTH;
	int yy = py;
	// Put at same Y placement as parent, unless we don't fit
	if (yy + get_height() + 20 > hh) yy = hh - get_height() - 20;
	Util::fix_window_coords(*this, xx, yy); // Sanity check
	move(xx,yy);
}

Movie::Progress::~Progress() {}

void Movie::Progress::chunk_done(Plot3::Plot3Chunk* job) {
	gdk_threads_enter();
	chunk_done_gdklocked(job);
	gdk_threads_leave();
}
void Movie::Progress::chunk_done_gdklocked(Plot3::Plot3Chunk* job) {
	if (job == 0)
		chunks_done = 0;
	else
		++chunks_done;
}
void Movie::Progress::pass_complete(std::string& msg, unsigned passes_plotted, unsigned maxiter, unsigned pixels_still_live, unsigned total_pixels) {
	gdk_threads_enter();
	pass_complete_gdklocked(msg, passes_plotted, maxiter, pixels_still_live, total_pixels);
	gdk_threads_leave();
}
void Movie::Progress::pass_complete_gdklocked(std::string& msg, unsigned /*passes_plotted*/, unsigned /*maxiter*/, unsigned pixels_still_live, unsigned total_pixels) {
	framebar->set_fraction( (double) (total_pixels - pixels_still_live) / total_pixels);
	framebar->set_text(msg);
	chunk_done_gdklocked(0 /* indicates start-of-pass */);
	update_chunks_bar_gdklocked();
}
void Movie::Progress::plot_complete() {
	frames_traversed(1);
}
void Movie::Progress::frames_traversed(int n) {
	gdk_threads_enter();
	frames_traversed_gdklocked(n);
	gdk_threads_leave();
}
void Movie::Progress::frames_traversed_gdklocked(int n) {
	frames_done += n;

	std::ostringstream msg1;
	msg1 << "0 passes plotted; maxiter = 0; " << npixels << " pixels live";
	std::string str1(msg1.str());

	std::ostringstream msg2;
	msg2 << frames_done << "/" << nframes << " frames";
	moviebar->set_text(msg2.str());
	moviebar->set_fraction((double)frames_done / nframes);

	pass_complete_gdklocked(str1, 0, 0, npixels, npixels);
}
void Movie::Progress::set_chunks_count(int n) {
	chunks_count = n;
}
bool Movie::Progress::on_timer() {
	gdk_threads_enter();
	update_chunks_bar_gdklocked();
	gdk_threads_leave();
	return true;
}
void Movie::Progress::update_chunks_bar_gdklocked() {
	std::ostringstream msg1;
	msg1 << chunks_done;
	if (chunks_count > 0)
		msg1 << " / " << chunks_count;
	msg1 <<	" chunks done";

	if (chunks_count > 0)
		plotbar->set_fraction((double)chunks_done / chunks_count);
	else
		plotbar->pulse();
	plotbar->set_text(msg1.str());
}
void Movie::Progress::do_cancel() {
	cancel_btn->set_label("Cancel requested");
	moviebar->set_text("Cancelling at the end of current frame");
	renderer.request_cancel();
}
bool Movie::Progress::on_delete_event(GdkEventAny *) {
	Gtk::MessageDialog dialog(*this, "Cancel this movie render?", false, Gtk::MessageType::MESSAGE_WARNING, Gtk::ButtonsType::BUTTONS_YES_NO, true);
	int response = dialog.run();
	if (response == Gtk::ResponseType::RESPONSE_NO)
		return true;
	do_cancel();
	return false;
}
