/*
    IMovieProgress.h: Movie progress reporter interface
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

#ifndef IMOVIEPROGRESS_H
#define IMOVIEPROGRESS_H

#include "IPlot3DataSink.h"

namespace Movie {

class RenderJob;
class IMovieCompleteHandler {
	public:
		// Signals that the current movie job has been completed.
		virtual void signal_completion(RenderJob& job) = 0;
		virtual void signal_error(RenderJob& job, const std::string& message) = 0;
		virtual ~IMovieCompleteHandler() {}
};

class IMovieProgressReporter : public Plot3::IPlot3DataSink {
	public:
		// Setup. Not mandatory, we do the best we can if not called.
		virtual void set_chunks_count(int n) = 0;
		// Call when outputting multiple frames at once. We record @n@ frames as being plotted.
		// Beware, there is an off-by-one error: plot_complete is called for every frame rendered,
		// so when a renderer copies its output, frames_traversed should be called with n-1.
		virtual void frames_traversed(int n) = 0;

		// We also inherit from  Plot3::IPlot3DataSink:
		// virtual void chunk_done(Plot3::Plot3Chunk* job) = 0;
		// virtual void pass_complete(std::string& msg, unsigned passes_plotted, unsigned maxiter, unsigned pixels_still_live, unsigned total_pixels) = 0;
		// virtual void plot_complete() = 0; // One plot = one FRAME of the movie.

		virtual ~IMovieProgressReporter() {}
};

// The null progress reporter does nothing.
class MovieNullProgress : public IMovieProgressReporter {
	public:
		MovieNullProgress();
		virtual ~MovieNullProgress();

		virtual void set_chunks_count(int);
		virtual void frames_traversed(int);
		virtual void chunk_done(Plot3::Plot3Chunk*);
		virtual void pass_complete(std::string&, unsigned passes_plotted, unsigned maxiter, unsigned pixels_still_live, unsigned total_pixels);
		virtual void plot_complete();
};

}; // namespace Movie

#endif // IMOVIEPROGRESS_H
