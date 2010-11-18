/*
    Plot2.cpp: Fractal plotting engine (second version)
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

#include <glib.h>
#include <assert.h>
#include "Plot2.h"

#define MAX_WORKER_THREADS 3
#define N_WORKER_JOBS 10
// TODO: Make these configurable.

static gpointer plot2_main_threadfunc(gpointer arg);
static void plot2_worker_threadfunc(gpointer data1, gpointer data2);

using namespace std;

#define LOCKP(p2) do { g_static_mutex_lock(&p2->lock); } while(0)
#define LOCK() LOCKP(this)

#define UNLOCKP(p2) do { g_static_mutex_unlock(&p2->lock); } while(0)
#define UNLOCK() UNLOCKP(this)

Plot2::Plot2(Fractal* f, cdbl centre, cdbl size,
		unsigned maxiter, unsigned width, unsigned height) :
		fract(f), centre(centre), size(size), maxiter(maxiter),
		width(width), height(height),
		main_thread(0), pool(0), callback(0), _data(0), _abort(false)
{
	g_static_mutex_init(&lock);
	flare = g_cond_new();
	flare_lock = g_mutex_new();
}

string Plot2::info(bool verbose) {
	std::ostringstream rv;
	rv.precision(MAXIMAL_DECIMAL_PRECISION);
	rv << fract->name
	   << "@(" << real(centre) << ", " << imag(centre) << ")";
	rv << ( verbose ? ", maxiter=" : " max=");
	rv << maxiter;

	// Now that we autofix the aspect ratio, our pixels are square.
	double zoom = 1.0/real(size);

	rv.precision(4); // Don't need more than this for the axis length or pixsize.
	if (verbose) {
		char buf[128];
		unsigned rr = snprintf(buf, sizeof buf, "%g", zoom);
		assert (rr < sizeof buf);
		rv << ", zoom=" << buf;
		rv << " / axis length=" << size << " / pixel size=" << real(size)/width;
	} else {
		rv << " axis=" <<size;
	}

	return rv.str();
}

/* Starts a plot. A thread is spawned to do the actual work. */
GError* Plot2::start(callback_t* c) {
	LOCK();
	assert(!main_thread);
	callback = c;
	GError * err = 0;
	main_thread = g_thread_create(plot2_main_threadfunc, this, true, &err);
	if (!main_thread) {
		fprintf(stderr, "Could not start main render thread: %d: %s\n", err->code, err->message);
		return err;
	}
	UNLOCK();
	return 0;
}

static gpointer plot2_main_threadfunc(gpointer arg) {
	Plot2 * plot = (Plot2*) arg;
	return plot->_main_threadfunc();
}

class Plot2::worker_job {
	friend class Plot2;
	unsigned maxiter, first_row, n_rows;
	bool done;
	worker_job() {};
	void set(const unsigned& max, const unsigned& first, const unsigned& n) {
		maxiter=max; first_row=first; n_rows=n;
	}
};

gpointer Plot2::_main_threadfunc()
{
	int i;
	LOCK();
	if (_data) delete[] _data;
	_data = new fractal_point[width * height];
	UNLOCK();

	GError * err = 0;
	assert(!pool);
	pool = g_thread_pool_new(plot2_worker_threadfunc, this, MAX_WORKER_THREADS, true, &err);
	if (!pool) {
		fprintf(stderr, "Could not start render thread pool: %d: %s\n", err->code, err->message);
		abort(); // TODO: better error handling.
	}

	// TODO: PREPARE the fractal (set all points to pass=1, Re, Im; precomp for cardoid etc.)

	worker_job jobs[N_WORKER_JOBS];
	const unsigned step = height / N_WORKER_JOBS;

	for (i=0; i<N_WORKER_JOBS; i++) {
		const unsigned z = step*i;
		jobs[i].set(maxiter, z, step);
		g_thread_pool_push(pool, &jobs[i], 0);
	}

	int todo;
	g_mutex_lock(flare_lock);
	do {
		g_cond_wait(flare, flare_lock);
		// TODO ALLOW STOP.
		// XXX: timed wait ?
		todo = g_thread_pool_unprocessed(pool);
	} while (todo);
	g_mutex_unlock(flare_lock);

	// XXX: CALL the callback (if present), LOOP for further passes...

	g_thread_pool_free(pool, false, true);
	pool = 0;

	return 0;
}

static void plot2_worker_threadfunc(gpointer data, gpointer user_data) {
	Plot2 * plot = (Plot2*) user_data;
	Plot2::worker_job * job = (Plot2::worker_job*) data;
	plot->_worker_threadfunc(job);
}

void Plot2::_worker_threadfunc(worker_job * job) {
	const unsigned firstrow = job->first_row;
	unsigned n_rows = job->n_rows;

	unsigned i,j;
	const cdbl _origin = origin(); // origin of the _whole plot_, not of firstrow
	//std::cout << "render centre " << centre << "; size " << size << "; origin " << origin << std::endl;

	// Sanity check: don't overrun the plot.
	if (firstrow + n_rows > height)
		n_rows = height - firstrow;
	const unsigned endpoint = firstrow + n_rows;

	cdbl colstep = cdbl(real(size) / width,0);
	cdbl rowstep = cdbl(0, imag(size) / height);
	//std::cout << "rowstep " << rowstep << "; colstep "<<colstep << std::endl;

	cdbl render_point = _origin;
	render_point.imag(render_point.imag() + firstrow*imag(rowstep));

	unsigned out_index = firstrow * width;
	// keep running points.
	for (j=firstrow; j<endpoint; j++) {
		for (i=0; i<width; i++) {
			fract->plot_pixel(render_point, maxiter, _data[out_index]);
			//std::cout << "Plot " << render_point << " i=" << out->iter << std::endl;
			++out_index;
			render_point += colstep;
		}
		render_point.real(real(_origin));
		render_point += rowstep;
	}

	signal();
}

/* Blocks, waiting for all work to finish. It may be some time! */
int Plot2::wait() {
	if (!main_thread)
		return -1;

	g_thread_join(main_thread);
	LOCK();
	main_thread = 0;
	UNLOCK();
	return 0;
}

/* Instructs the running plot to stop what it's doing ASAP.
 * Blocks until it has done so, which should be fairly quick. */
int Plot2::stop() {
	_abort = true;
	// TODO: TELL IT TO STOP - notify in some way ? XXX //
	return wait();
}

/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates */
cdbl Plot2::pixel_to_set(int x, int y)
{
	if (x<0) x=0; else if ((unsigned)x>width) x=width;
	if (y<0) y=0; else if ((unsigned)y>height) y=height;

	const double pixwide = real(size) / width,
		  		 pixhigh  = imag(size) / height;
	cdbl delta (x*pixwide, y*pixhigh);
	return origin() + delta;
}

Plot2::~Plot2() {
	stop();
	delete[] _data;
	// TODO: Anything else to free?
	LOCK();
	assert(!main_thread);
	g_static_mutex_free(&lock);
	g_cond_free(flare);
	g_mutex_free(flare_lock);
	assert (pool==0);
}
