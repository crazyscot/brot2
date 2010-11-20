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

#include <glib.h>
#include <glibmm.h>
#include "Fractal.h"


class Plot2 {
public:
	/* Callback type. */
	class callback_t {
	public:
		/* The plotting interface calls back when it has finished a plotting pass
		 * and might want to do some more. The caller may for example wish to
		 * update the display; the plot array is guaranteed not to update under
		 * your feet until you return from the callback.
		 */
		virtual void plot_pass_complete(Plot2& plot) = 0;

		/* Notification when the plotting is really finished or on stop(). */
		virtual void plot_complete(Plot2& plot) = 0;
	};

	/* What is this plot about? */
	const Fractal* fract;
	const cdbl centre, size;
	const unsigned maxiter;
	const unsigned width, height; // plot size in pixels
	const cdbl origin() const { return centre - size/(long double)2.0; }

	// Returns a human-readable summary of this plot for the status bar.
	virtual std::string info(bool verbose = false);

	/* The constructor may request the fractal to do any precomputation
	 * necessary (known-blank regions, for example). */
	Plot2(Fractal* f, cdbl centre, cdbl size, unsigned maxiter, unsigned width, unsigned height);
	virtual ~Plot2();

	/* Starts a plot. A thread is spawned to do the actual work.
	 * Returns NULL on success, otherwise a GError describing the problem. */
	GError* start(callback_t* c);

	/* Blocks, waiting for all work to finish. It may be some time! */
	int wait();

	/* Instructs the running plot to stop what it's doing ASAP.
	 * Does NOT block; the plot may carry on for a little while.
	 * If this is a problem, call wait() as well. */
	void stop();

	/* Read-only access to the plot data. */
	const fractal_point * get_data() { return _data; }

	/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
	 * Returns 1 for success, 0 if the point was outside of the render.
	 * N.B. that we assume that pixel co-ordinates have a bottom-left origin! */
	cdbl pixel_to_set(int x, int y);

	/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates.
	 * Returns 1 for success, 0 if the point was outside of the render.
	 * This is a variant form for pixels with a top-left origin, such as
	 * those of gtk/gdk. */
	cdbl pixel_to_set_tlo(int xx, int yy) {
		return pixel_to_set(xx, height-yy-1);
	};

	// Internal use only.
	gpointer _main_threadfunc();
	class worker_job; // Opaque.
	void _worker_threadfunc(worker_job * job);

private:
	GStaticMutex lock;
	GThread * main_thread; // Access protected by lock.
	GCond * flare; // For individual worker threads to signal completion
	GMutex * flare_lock;

	Glib::ThreadPool * tpool; // Thread pool to use. Set up by constructor.

	callback_t* callback;
	fractal_point* _data;
	volatile bool _abort;
	int outstanding; // How many jobs are there? Protected by flare_lock.
	// XXX TODO: Can we merge lock and flare_lock?

	// Calls to wake up the main thread. May optionally decrement the outstanding-jobs counter, which is protected by the same lock.
	inline void awaken(bool job_complete=false) {
		g_mutex_lock(flare_lock);
		if (job_complete) --outstanding;
		g_cond_broadcast(flare);
		g_mutex_unlock(flare_lock);
	};
};


#endif /* PLOT2_H_ */
