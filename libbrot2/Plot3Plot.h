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

class Prefs;

namespace Plot3 {

class Plot3Plot {
public:
	// TODO: Callbacks maj/min/complete - what are we donig with them?

	/* What is this plot about? */
	ThreadPool& _pool;
	IPlot3DataSink* sink;
	const Fractal::FractalImpl& fract;
	const ChunkDivider::Base& divider;

	const Fractal::Point centre, size; // Centre co-ordinates; axis length
	const unsigned width, height; // plot size in pixels
	const Fractal::Point origin() const { return centre - size/(Fractal::Value)2.0; }

	// Returns a human-readable summary of this plot for the status bar.
	virtual std::string info(bool verbose = false) const;

	/* No assignment or copy constructor. */
	Plot3Plot(Plot3Plot&) = delete;
	const Plot3Plot& operator=( const Plot3Plot& ) = delete;

	/* The real constructor may request the fractal to do any precomputation
	 * necessary (known-blank regions, for example). */
	Plot3Plot(ThreadPool& pool, IPlot3DataSink* s, Fractal::FractalImpl& f, ChunkDivider::Base& div,
			Fractal::Point centre, Fractal::Point size, unsigned width, unsigned height, unsigned max_passes=0);
	virtual ~Plot3Plot();

	/* Starts a plot. The real work goes on asynchronously. */
	void start();
	// TODO: Resume ? Is this part of start()?

	/* Instructs the running plot to stop what it's doing ASAP.
	 * Does NOT block; the plot may carry on for a little while. */
	void stop();

	/**
	 * Stops the plot ASAP, blocking until it has done so.
	 * NOTE that we first call the data sink's completion
	 * handler (if present) before returning.
	 * May return immediately if the plot was not running.
	 */
	void wait();

	unsigned chunks_total() const;

	// Provides a means to override the prefs.
	void set_prefs(std::shared_ptr<Prefs>& newprefs);
	void set_prefs(std::shared_ptr<const Prefs>& newprefs);

protected:
	std::shared_ptr<const Prefs> prefs; // Where to get our global settings from.

	bool _shutdown; // Set only when we are being deleted. PROTECT by _lock !
	bool _running; // Set when we are running. PROTECT by _lock !
	bool _stop; // Set to ask the running plot to stop. PROTECT by _lock !

	/* Plot statistics: */
	int plotted_maxiter; // How far did we get before bailing?
	int plotted_passes; // How many passes before bailing?
	unsigned passes_max; // Do we have an absolute limit on the number of passes?

	void threadfunc(); // Worker thread function
	void run(); // Actually does the work

private:
	std::list<Plot3Chunk*> _chunks;

	/* Message passing between threads within the class */
	std::mutex _lock;
	std::atomic<int> _runs_sema; // Protected by _lock, notify by _runs_cond
	std::condition_variable _runs_cond; // Protected by _lock
	std::condition_variable _completion_cond; // Also protected by _lock

	std::thread runner;

	void poke_runner();

public:
	// Wormhole to access the chunks list. Calls wait() first.
	const std::list<Plot3Chunk*>& get_chunks__only_after_completion();
};

} // namespace Plot3

#endif /* PLOT3_H_ */
