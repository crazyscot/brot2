/*
    Plot2.h: Fractal plotting engine (second version)
    Copyright (C) 2010 Ross Younger

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

#ifndef PLOT2_H_
#define PLOT2_H_

#include <memory>
#include <glibmm.h>
#include "noncopyable.hpp"
#include "Fractal.h"

class Prefs;

class Plot2 : boost::noncopyable {
public:
	/* Callback type. */
	class callback_t {
	public:
		/* Report that we did something. Minor progress, in other words.
		 * BEWARE that the worker threads are all still beavering away
		 * during this call, so take great care if you want to look at
		 * the render data!
		 * 'workdone' identifies the work done in the current
		 * pass, from 0 = nothing to 1 = complete.
		 */
		virtual void plot_progress_minor(Plot2& plot, float workdone) = 0;

		/* Report major progress, typically that we've finished one pass
		 * but might want to make more.
		 * The current limit of iteration is given as current_maxiter.
		 * The caller may wish to update the display; the data array is
		 * guaranteed not to update under your feet until you return from
		 * the callback.
		 */
		virtual void plot_progress_major(Plot2& plot, unsigned current_maxiter, std::string& commentary) = 0;

		/* Notification that plotting is really finished.
		 * Note that this does NOT get called if the plot run has been aborted! */
		virtual void plot_progress_complete(Plot2& plot) = 0;

	protected:
		~callback_t() {}
	};

	/* What is this plot about? */
	const Fractal::FractalImpl* fract;
	const Fractal::Point centre, size; // Centre co-ordinates; axis length
	const unsigned width, height; // plot size in pixels
	const Fractal::Point origin() const { return centre - size/(Fractal::Value)2.0; }

	// Returns a human-readable summary of this plot for the status bar.
	virtual std::string info(bool verbose = false) const;

	/* The constructor may request the fractal to do any precomputation
	 * necessary (known-blank regions, for example). */
	Plot2(Fractal::FractalImpl* f, Fractal::Point centre, Fractal::Point size, unsigned width, unsigned height, unsigned max_passes=0);
	virtual ~Plot2();

	/* Starts a plot. A thread is spawned to do the actual work.
	 * Throws an exception (from Glib::Thread) if something went wrong. */
	void start(callback_t* c, bool is_resume = false);

	/* Blocks, waiting for all work to finish. It may be some time! */
	int wait();

	/* Instructs the running plot to stop what it's doing ASAP.
	 * Does NOT block; the plot may carry on for a little while.
	 * If this is a problem, call wait() as well. */
	void stop();

	/* Read-only access to the plot data. */
	const Fractal::PointData * get_data() { if (!this) return 0; return _data; }

	/* Returns data for a single point, identified by its pixel co-ordinates within the plot.
	 * (NB. This function does not know about antialiasing! Use the
	 * Render equivalent.) */
	const Fractal::PointData& get_pixel_point(int x, int y);

	/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
	 * Returns 1 for success, 0 if the point was outside of the render.
	 * N.B. that we assume that pixel co-ordinates have a bottom-left origin! */
	Fractal::Point pixel_to_set(int x, int y) const;

	/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
	 * Returns 1 for success, 0 if the point was outside of the render.
	 * This is a variant form for pixels with a top-left origin, such as
	 * those of gtk/gdk. */
	Fractal::Point pixel_to_set_tlo(int xx, int yy) const {
		return pixel_to_set(xx, height-yy-1);
	};

	/* What iteration count did we bail out at? */
	int get_maxiter() const { return plotted_maxiter; };
	/* How many passes before we bailed out? */
	int get_passes() const { return plotted_passes; };

	/* Are we there yet? */
	inline bool is_done() {
		Glib::Mutex::Lock _auto(plot_lock);
		return _done;
	}

	// Provides a means to override the prefs.
	void set_prefs(std::shared_ptr<Prefs>& newprefs);
	void set_prefs(std::shared_ptr<const Prefs>& newprefs);

protected:
	/* Prepares a plot: creates the _data array and asks the fractal to
	 * initialise it.
	 * THIS FUNCTION WILL BE CALLED WITH plot_lock HELD. */
	void prepare();

	/* Plot completion detection: */
	std::shared_ptr<const Prefs> prefs; // Where to get our global settings from. This is requeried on prepare().
	unsigned initial_maxiter; // Iteration limit on first pass
	double live_threshold; // Proportion of the pixels that must escape in a pass; if less, we consider stopping
	unsigned minimum_escapee_percent; // Minimum %age of pixels that must be done in order to consider stopping

	/* Plot statistics: */
	int plotted_maxiter; // How far did we get before bailing?
	int plotted_passes; // How many passes before bailing?
	unsigned passes_max; // Do we have an absolute limit on the number of passes?

private:
	Glib::Mutex plot_lock;
	Glib::Cond _worker_signal; // For individual worker threads to signal completion
	Glib::Cond _plot_complete; // For the per-plot "main" thread to signal completion, also protected by plot_lock

	callback_t* callback;  // Written few times, read many.
	Fractal::PointData* _data;  // Concurrently written by worker threads. Beware! (N.B. We own this pointer.)
	volatile bool _abort, _done; // Protected by plot_lock
	unsigned _outstanding; // How many jobs are live? Protected by plot_lock.
	unsigned _completed; // How many jobs are finished? Protected by plot_lock.

	class worker_job; // Opaque to Plot2.cpp.
	void _per_plot_threadfunc();
	void _worker_threadfunc(worker_job * job);

	// Called to wake up the plot's master thread.
	// May optionally decrement the outstanding-jobs counter (which is protected by the same lock).
	inline void awaken(bool job_complete=false) {
		Glib::Mutex::Lock _auto(plot_lock);
		if (job_complete) {
			--_outstanding;
			++_completed;
		}
		_worker_signal.broadcast();
	};

	class worker_job;
	worker_job *jobs;
};


#endif /* PLOT2_H_ */
