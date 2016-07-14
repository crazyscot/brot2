/*
    Plot3.h: Fractal plotting engine (third version)
    Copyright (C) 2012 Ross Younger

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

#ifndef PLOT3_H_
#define PLOT3_H_

#include <thread>
#include <queue>
#include "Fractal.h"
#include "Plot3Chunk.h"
#include "Plot3Pass.h"
#include "IPlot3DataSink.h"
#include "ChunkDivider.h"

namespace BrotPrefs {
class Prefs;
}

namespace Plot3 {

class Plot3Plot {
public:
	/* What is this plot about? */
	ThreadPool& _pool;
	IPlot3DataSink* sink;
	const Fractal::FractalImpl& fract;
	ChunkDivider::Base& divider;

	const Fractal::Point centre, size; // Centre co-ordinates; axis length
	const unsigned width, height; // plot size in pixels
	const Fractal::Point origin() const { return centre - size/(Fractal::Value)2.0; }
	Fractal::Value zoom() const;
	Fractal::Value axis_to_zoom(Fractal::Value ax) const;
	// zoom-to-axis is the same calculation but we'll provide the sugar anyway.
	Fractal::Value zoom_to_axis(Fractal::Value zo) const { return axis_to_zoom(zo); }

	// Returns a human-readable summary of this plot for the status bar.
	virtual std::string info(bool verbose = false) const;
	// The same, but just the zoom.
	virtual std::string info_zoom(bool show_legend = true) const;

	/* No assignment or copy constructor. */
	Plot3Plot(Plot3Plot&) = delete;
	const Plot3Plot& operator=( const Plot3Plot& ) = delete;

	/* The real constructor may request the fractal to do any precomputation
	 * necessary (known-blank regions, for example). */
	Plot3Plot(ThreadPool& pool, IPlot3DataSink* s, Fractal::FractalImpl& f, ChunkDivider::Base& div,
			Fractal::Point centre, Fractal::Point size, unsigned width, unsigned height, unsigned max_passes=0);
	virtual ~Plot3Plot();

	/* Starts a plot. The real work goes on asynchronously.
	 * The maths type to use is automatically determined
	 * from the plot centre and size. */
	void start();
	// Could maybe do a resume() - or is it part of start()?

	/* Starts a plot with an explicit maths type. */
	void start(Fractal::Maths::MathsType arith);

	/* Instructs the running plot to stop what it's doing ASAP.
	 * Does NOT block; the plot may carry on for a little while. */
	void stop();

	/**
	 * Waits for the plot to complete. Blocking.
	 * NOTE that we call the data sink's completion
	 * handler (if present) before returning.
	 * May return immediately if the plot was not running.
	 */
	void wait();

	unsigned chunks_total() const;
	unsigned get_passes() const { return plotted_passes; }
	int get_maxiter() const { return plotted_maxiter; }
	bool is_running();

	// Provides a means to override the prefs.
	void set_prefs(std::shared_ptr<BrotPrefs::Prefs>& newprefs);
	void set_prefs(std::shared_ptr<const BrotPrefs::Prefs>& newprefs);

	/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
	 * Returns 1 for success, 0 if the point was outside of the render.
	 * N.B. that we assume that pixel co-ordinates have a bottom-left origin! */
	Fractal::Point pixel_to_set_blo(int x, int y) const;

	/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
	 * Returns 1 for success, 0 if the point was outside of the render.
	 * This is a variant form for pixels with a top-left origin, such as
	 * those of gtk/gdk. */
	Fractal::Point pixel_to_set_tlo(int xx, int yy) const {
		return pixel_to_set_blo(xx, height-yy);
	};

protected:
	std::shared_ptr<const BrotPrefs::Prefs> prefs; // Where to get our global settings from.

	bool _shutdown; // Set only when we are being deleted. PROTECT by _lock !
	bool _running; // Set when we are running. PROTECT by _lock !
	bool _stop; // Set to ask the running plot to stop. PROTECT by _lock !

	/* Plot statistics: */
	int plotted_maxiter; // How far did we get before bailing?
	int plotted_passes; // How many passes before bailing?
	unsigned passes_max; // Do we have an absolute limit on the number of passes?

	void run(); // Actually does the work. Runs in its own thread (set up by constructor, called on start()).

private:
	std::list<Plot3Chunk*> _chunks;

	/* Message passing between threads within the class */
	std::mutex _lock;
	std::condition_variable _waiters_cond; // Protected by _lock. For anybody wait()ing on us to finish.

	static ThreadPool runner_pool; // LP#1099061: A single threaded "pool", runs our plot threads.
	std::future<void> completion; // Use get() in the destructor, to ensure all jobs finished. Callers should use wait().

public:
	// Wormhole to access the chunks list. Calls wait() first.
	const std::list<Plot3Chunk*>& get_chunks__only_after_completion();
};

} // namespace Plot3

#endif /* PLOT3_H_ */
