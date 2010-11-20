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

#include <glibmm.h>
#include <assert.h>
#include "Plot2.h"

#define MAX_WORKER_THREADS 2
#define N_WORKER_JOBS 20

class SingletonThreadPool {
	const int n_threads;
	const bool is_excl;
	const int max_unused;
	Glib::StaticMutex mux;
	Glib::ThreadPool * pool;
public:
	SingletonThreadPool(int n, bool excl, int max_idle=-1) : n_threads(n), is_excl(excl), max_unused(max_idle) {};
	Glib::ThreadPool * get() {
		/* I would love to instantiate the pool in the constructor, but
		 * it explodes if the glib thread subsystem isn't up yet...
		 */
		mux.lock();
		if (!pool) {
			pool = new Glib::ThreadPool(n_threads, is_excl);
			pool->set_max_unused_threads(max_unused);
		}
		mux.unlock();
		return pool;
	};
	virtual ~SingletonThreadPool() {
		mux.lock();
		if (pool) {
			delete pool;
			pool=0;
		}
	};
};

// Pool for the main per-plot threads
static SingletonThreadPool per_plot_thread_pool(-1, false, 2);

// Pool for the worker threads.
// Hard limit of outstanding jobs so we don't cane the CPU unnecessarily.
static SingletonThreadPool worker_thread_pool(MAX_WORKER_THREADS, true);

using namespace std;

Plot2::Plot2(Fractal* f, cdbl centre, cdbl size,
		unsigned maxiter, unsigned width, unsigned height) :
		fract(f), centre(centre), size(size), maxiter(maxiter),
		width(width), height(height),
		callback(0), _data(0), _abort(false), _done(false), _outstanding(0)
{
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
void Plot2::start(callback_t* c) {
	Glib::Mutex::Lock _lock(plot_lock);
	assert(!_done);
	_done = _abort = false; // Must do this here, rather than in main_threadfunc, to kill a race (if user double-clicks i.e. we get two do_redraws in quick succession).
	callback = c;
	per_plot_thread_pool.get()->push(sigc::mem_fun(this, &Plot2::_per_plot_threadfunc));
}

class Plot2::worker_job {
	friend class Plot2;
	Plot2* plot;
	unsigned maxiter, first_row, n_rows;
	worker_job() {};
	void set(Plot2* p, const unsigned& max, const unsigned& first, const unsigned& n) {
		plot = p; maxiter=max; first_row=first; n_rows=n;
	}
	void run() {
		assert(this);
		assert(plot);
		plot->_worker_threadfunc(this);
	}
};

void Plot2::_per_plot_threadfunc()
{
	int i;
	Glib::Mutex::Lock _auto (plot_lock);
	if (_data) delete[] _data;
	_data = new fractal_point[width * height];

	// TODO: PREPARE the fractal (set all points to pass=1, Re, Im; precomp for cardoid etc.)

	worker_job jobs[N_WORKER_JOBS];
	const unsigned step = (height + N_WORKER_JOBS - 1) / N_WORKER_JOBS;
	// Must round up to avoid a gap.

	for (i=0; i<N_WORKER_JOBS; i++) {
		const unsigned z = step*i;
		jobs[i].set(this, maxiter, z, step);
	}

	int out_ptr = 0;

	while (out_ptr < N_WORKER_JOBS && !_abort) {
		while (_outstanding < MAX_WORKER_THREADS && out_ptr < N_WORKER_JOBS) {
			++_outstanding; // plot_lock is held.
			worker_thread_pool.get()->push(sigc::mem_fun(jobs[out_ptr++], &worker_job::run));
			//printf("spawning, outstanding:=%d, out_ptr:=%d\n",outstanding,out_ptr);
		}
		_worker_signal.wait(plot_lock); // Signals that a job has completed, or we're asked to abort.
		if (callback) {
			_auto.release();
			callback->plot_progress_minor(*this);
			_auto.acquire();
		}
	};
	// Now wait for them to finish.
	while(_outstanding) {
		//printf("waiting, o=%d\n",outstanding);
		_worker_signal.wait(plot_lock);
		if (callback) {
			_auto.release();
			callback->plot_progress_minor(*this);
			_auto.acquire();
		}
	};

	// TODO, on multi-pass render: // if (callback) callback->plot_progress_major(*this);

	if (callback && !_abort) {
		_auto.release();
		callback->plot_progress_complete(*this);
		_auto.acquire();
	}
	_done = true;
	_plot_complete.broadcast();
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

	awaken(true);
}

/* Blocks, waiting for all work to finish. It may be some time! */
int Plot2::wait() {
	Glib::Mutex::Lock _auto(plot_lock);
	if (_done) return 0;
	_plot_complete.wait(plot_lock);
	return 0;
}

/* Instructs the running plot to stop what it's doing ASAP.
 * Blocks until it has done so, which should be fairly quick. */
void Plot2::stop() {
	{
		Glib::Mutex::Lock _auto(plot_lock);
		_abort = true;
	}
	awaken(false);
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
	wait();
	// TODO: Anything else to free?
	{
		Glib::Mutex::Lock _auto(plot_lock);
		delete[] _data;
		assert(_done);
	}
}
