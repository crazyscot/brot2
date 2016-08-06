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
#include <sstream>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/messagedialog.h>

Movie::Progress::Progress(const struct MovieInfo &_movie, Movie::Renderer& _ren) : renderer(_ren),
	chunks_done(0), chunks_count(0), frames_done(-1),
	npixels(_movie.height * _movie.width * (_movie.antialias ? 4 : 1)),
	nframes(_movie.count_frames()), movie(_movie)
{
	gdk_threads_enter();
	vbox = Gtk::manage(new Gtk::VBox());
	plotbar = Gtk::manage(new Gtk::ProgressBar());
	framebar = Gtk::manage(new Gtk::ProgressBar());
	moviebar = Gtk::manage(new Gtk::ProgressBar());
	set_title("Movie progress");
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

	Gtk::Button *cancel_btn = Gtk::manage(new Gtk::Button("Cancel"));
	cancel_btn->signal_clicked().connect(sigc::mem_fun(*this, &Movie::Progress::do_cancel));
	bbox->pack_end(*cancel_btn);

	add(*vbox);
	show_all();
	Glib::signal_timeout().connect( sigc::mem_fun(*this, &Movie::Progress::on_timer), 300 );
	gdk_threads_leave();
	plot_complete(); // Resets frames_done to 0
}

Movie::Progress::~Progress() {}

void Movie::Progress::chunk_done(Plot3::Plot3Chunk* job) {
	gdk_threads_enter();
	if (job == 0)
		chunks_done = 0;
	else
		++chunks_done;
	gdk_threads_leave();
}
void Movie::Progress::pass_complete(std::string& msg, unsigned /*passes_plotted*/, unsigned /*maxiter*/, unsigned pixels_still_live, unsigned total_pixels) {
	gdk_threads_enter();
	framebar->set_fraction( (double) (total_pixels - pixels_still_live) / total_pixels);
	framebar->set_text(msg);
	gdk_threads_leave();
	chunk_done(0 /* indicates start-of-pass */);
	on_timer(); // force that bar to update
}
void Movie::Progress::plot_complete() {
	frames_traversed(1);
}
void Movie::Progress::frames_traversed(int n) {
	frames_done += n;

	std::ostringstream msg1;
	msg1 << "0 passes plotted; maxiter = 0; " << npixels << " pixels live";
	std::string str1(msg1.str());

	gdk_threads_enter();
	std::ostringstream msg2;
	msg2 << frames_done << "/" << nframes << " frames";
	moviebar->set_text(msg2.str());
	moviebar->set_fraction((double)frames_done / nframes);
	gdk_threads_leave();

	pass_complete(str1, 0, 0, npixels, npixels);
}
void Movie::Progress::set_chunks_count(int n) {
	chunks_count = n;
}
bool Movie::Progress::on_timer() {
	std::ostringstream msg1;
	msg1 << chunks_done;
	if (chunks_count > 0)
		msg1 << " / " << chunks_count;
	msg1 <<	" chunks done";

	gdk_threads_enter();
	if (chunks_count > 0)
		plotbar->set_fraction((double)chunks_done / chunks_count);
	else
		plotbar->pulse();
	plotbar->set_text(msg1.str());
	gdk_threads_leave();
	return true;
}
void Movie::Progress::do_cancel() {
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
