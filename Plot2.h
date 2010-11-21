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

#include <glibmm.h>
#include "Fractal.h"


class Plot2 {
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
	};

	/* What is this plot about? */
	const Fractal* fract;
	const cfpt centre, size;
	const unsigned width, height; // plot size in pixels
	const cfpt origin() const { return centre - size/(fvalue)2.0; }

	// Returns a human-readable summary of this plot for the status bar.
	virtual std::string info(bool verbose = false);

	/* The constructor may request the fractal to do any precomputation
	 * necessary (known-blank regions, for example). */
	Plot2(Fractal* f, cfpt centre, cfpt size, unsigned width, unsigned height);
	virtual ~Plot2();

	/* Starts a plot. A thread is spawned to do the actual work.
	 * Throws an exception (from Glib::Thread) if something went wrong. */
	void start(callback_t* c);

	/* Blocks, waiting for all work to finish. It may be some time! */
	int wait();

	/* Instructs the running plot to stop what it's doing ASAP.
	 * Does NOT block; the plot may carry on for a little while.
	 * If this is a problem, call wait() as well. */
	void stop();

	/* Read-only access to the plot data. */
	const fractal_point * get_data() { if (!this) return 0; return _data; }

	/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
	 * Returns 1 for success, 0 if the point was outside of the render.
	 * N.B. that we assume that pixel co-ordinates have a bottom-left origin! */
	cfpt pixel_to_set(int x, int y);

	/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
	 * Returns 1 for success, 0 if the point was outside of the render.
	 * This is a variant form for pixels with a top-left origin, such as
	 * those of gtk/gdk. */
	cfpt pixel_to_set_tlo(int xx, int yy) {
		return pixel_to_set(xx, height-yy-1);
	};

	/* What iteration count did we bail out at? */
	const int get_maxiter() { return plotted_maxiter; };
	/* How many passes before we bailed out? */
	const int get_passes() { return plotted_passes; };

protected:
	/* Prepares a plot: creates the _data array and asks the fractal to
	 * initialise it.
	 * THIS FUNCTION WILL BE CALLED WITH plot_lock HELD. */
	void prepare();

	int plotted_maxiter; // How far did we get before bailing?
	int plotted_passes; // How many passes before bailing?

private:
	Glib::Mutex plot_lock;
	Glib::Cond _worker_signal; // For individual worker threads to signal completion
	Glib::Cond _plot_complete; // For the per-plot "main" thread to signal completion, also protected by plot_lock

	callback_t* callback;  // Written few times, read many.
	fractal_point* _data;  // Concurrently written by worker threads. Beware! (N.B. We own this pointer.)
	volatile bool _abort, _done; // Protected by plot_lock
	unsigned _outstanding; // How many jobs remain? Protected by plot_lock.

	class worker_job; // Opaque to Plot2.cpp.
	void _per_plot_threadfunc();
	void _worker_threadfunc(worker_job * job);

	// Called to wake up the plot's master thread.
	// May optionally decrement the outstanding-jobs counter (which is protected by the same lock).
	inline void awaken(bool job_complete=false) {
		Glib::Mutex::Lock _auto(plot_lock);
		if (job_complete) --_outstanding;
		_worker_signal.broadcast();
	};
};


#endif /* PLOT2_H_ */
