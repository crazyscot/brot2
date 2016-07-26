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

Movie::Progress::Progress(const struct MovieInfo &_movie, const Movie::Renderer& _ren) : renderer(_ren),
	chunks_done(0), frames_done(-1),
	npixels(_movie.height * _movie.width * (_movie.antialias ? 4 : 1)),
	nframes(_movie.count_frames()), movie(_movie)
{
	gdk_threads_enter();
	set_title("Movie progress");
	Gtk::Label *lbl;
#define LABEL(_txt) do { lbl = Gtk::manage(new Gtk::Label(_txt)); vbox.pack_start(*lbl); } while(0)
	LABEL("Current pass");
	vbox.pack_start(plotbar);
	LABEL("Current plot");
	vbox.pack_start(framebar);
	LABEL("Overall");
	vbox.pack_start(moviebar);
	add(vbox);
	show_all();
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
	plotbar.pulse();
	// TODO plotbar.set_fraction((double)chunks_done / NCHUNKS); // No easy link to current plot
	gdk_threads_leave();
}
void Movie::Progress::pass_complete(std::string& msg) {
	gdk_threads_enter();
	framebar.pulse();
	framebar.set_text(msg);
	gdk_threads_leave();
	chunk_done(0 /* indicates start-of-pass */);
}
void Movie::Progress::plot_complete() {
	++frames_done;
	std::ostringstream msg1;
	msg1 << "0 passes plotted; maxiter = 0; " << npixels << " pixels live";
	std::string str1(msg1.str());

	gdk_threads_enter();
	std::ostringstream msg2;
	msg2 << "Frame " << frames_done << " of " << nframes;
	moviebar.set_text(msg2.str());
	moviebar.set_fraction((double)frames_done / nframes);
	gdk_threads_leave();

	pass_complete(str1);
}

