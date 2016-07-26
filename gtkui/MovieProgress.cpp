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
	set_title("Movie progress");
	Gtk::Label *lbl;
#define LABEL(_txt) do { lbl = Gtk::manage(new Gtk::Label(_txt)); vbox.pack_start(*lbl); } while(0)
	LABEL("Current pass");
	vbox.pack_start(plotbar);
	LABEL("Current plot");
	vbox.pack_start(framebar);
	LABEL("Overall");
	vbox.pack_start(moviebar);
	plot_complete(); // Resets frames_done to 0
	add(vbox);
	show_all();
}

Movie::Progress::~Progress() {}

void Movie::Progress::chunk_done(Plot3::Plot3Chunk* job) {
	if (job == 0)
		chunks_done = 0;
	else
		++chunks_done;
	plotbar.pulse();
	// TODO plotbar.set_fraction((double)chunks_done / NCHUNKS); // No easy link to current plot
}
void Movie::Progress::pass_complete(std::string& msg) {
	framebar.pulse();
	framebar.set_text(msg);
	chunk_done(0 /* indicates start-of-pass */);
}
void Movie::Progress::plot_complete() {
	++frames_done;
	{
		std::ostringstream msg;
		msg << "0 passes plotted; maxiter = 0; " << npixels << " pixels live";
		std::string str(msg.str());
		pass_complete(str);
	}
	moviebar.set_fraction((double)frames_done / nframes);

	{
		std::ostringstream msg;
		msg << "Frame " << frames_done << " of " << nframes;
		moviebar.set_text(msg.str());
	}
}

