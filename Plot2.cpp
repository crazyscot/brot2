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

using namespace std;

#define MAX_WORKER_THREADS 2

#define INITIAL_PASS_MAXITER 256
/* Judging the number of iterations and how to scale it on successive passes
 * is really tricky. Too many and it takes too long to get that first pass home;
 * too few (too slow growth) and you're wasting cycles keeping track of the
 * live count and your initial pass might be all-black anyway - not to mention
 * the chance of hitting the quality threshold sooner than you might have liked).
 */

/* We consider the plot to be complete when at least the minimum threshold
 * of the image has escaped, and the number of pixels escaping
 * in the last pass drops below some threshold of the whole picture size.
 * Beware that this can still lead to infinite loops if you look at regions
 * with excessive numbers of points within the set... */
#define LIVE_THRESHOLD_FRACT 0.001
#define MIN_ESCAPEE_PCT 2

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

Plot2::Plot2(Fractal* f, cfpt centre, cfpt size,
		unsigned width, unsigned height) :
		fract(f), centre(centre), size(size),
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
	rv << plotted_maxiter;

	// Now that we autofix the aspect ratio, our pixels are square.
	double zoom = 1.0/real(size); // not an fvalue, so we can print it

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
	unsigned maxiter, first_row, n_rows, live_pixels;
	worker_job() {};
	worker_job(Plot2* p, const unsigned first, const unsigned n) {
		plot = p; first_row=first; n_rows=n;
		live_pixels = plot->width * n_rows;
		/* This leads to an overestimate when the job exceeds beyond the
		 * plot boundary. We don't care, as it's only the _delta_ of this
		 * number that's interesting. */
	};
	void set(const unsigned& max) {
		maxiter = max;
	}
	void run() {
		assert(this);
		assert(plot);
		plot->_worker_threadfunc(this);
	}
};

void Plot2::prepare()
{
	// Called with plot_lock held!
	if (_data) delete[] _data;
	_data = new fractal_point[width * height];

	const cfpt _origin = origin(); // origin of the _whole plot_, not of firstrow
	unsigned i,j, out_index = 0;
	//std::cout << "render centre " << centre << "; size " << size << "; origin " << origin << std::endl;

	cfpt colstep = cfpt(real(size) / width,0);
	cfpt rowstep = cfpt(0, imag(size) / height);
	//std::cout << "rowstep " << rowstep << "; colstep "<<colstep << std::endl;
	cfpt render_point = _origin;

	for (j=0; j<height; j++) {
		for (i=0; i<width; i++) {
			fract->prepare_pixel(render_point, _data[out_index]);
			++out_index;
			render_point += colstep;
		}
		render_point.real(real(_origin));
		render_point += rowstep;
	}
}

void Plot2::_per_plot_threadfunc()
{
	unsigned i, passcount=0, delta_threshold = width * height * LIVE_THRESHOLD_FRACT;
	const unsigned NJOBS = height / (10000 / width + 1);
	Glib::Mutex::Lock _auto (plot_lock);
	bool live = true;

	prepare(); // Could push this into the job threads, if it were necessary.

	worker_job jobs[NJOBS];
	const unsigned step = (height + NJOBS - 1) / NJOBS;
	// Must round up to avoid a gap.

	for (i=0; i<NJOBS; i++)
		jobs[i] = worker_job(this, step*i, step);

	unsigned livecount=0, prev_livecount;
	for (i=0; i<NJOBS; i++) livecount += jobs[i].live_pixels;
	//printf("Initial livecount %u\n", livecount);

	int this_pass_maxiter = INITIAL_PASS_MAXITER, last_pass_maxiter = 0, maxiter_scale;
	do {
		++passcount;
		unsigned out_ptr = 0, jobsdone = 0;
		for (i=0; i<NJOBS; i++)
			jobs[i].set(this_pass_maxiter);

		while (out_ptr < NJOBS && !_abort) {
			while (_outstanding < MAX_WORKER_THREADS && out_ptr < NJOBS) {
				++_outstanding; // plot_lock is held.
				// TODO don't push a job if it has no live pixels?
				worker_thread_pool.get()->push(sigc::mem_fun(jobs[out_ptr++], &worker_job::run));
			}
			_worker_signal.wait(plot_lock); // Signals that a job has completed, or we're asked to abort.
			if (!_abort) {
				++jobsdone;
				if (callback) {
					_auto.release();
					callback->plot_progress_minor(*this, (float)jobsdone / NJOBS);
					_auto.acquire();
				}
			}
		};
		// Now wait for them to finish.
		while(_outstanding) {
			_worker_signal.wait(plot_lock);
			++jobsdone;
			if (callback) {
				_auto.release();
				callback->plot_progress_minor(*this, (float)jobsdone / NJOBS);
				_auto.acquire();
			}
		};

		// How many pixels are still live? Is it worth continuing?
		// We consider the plot to be complete when at least the minimum threshold
		// of the image has escaped, and the number of pixels escaping
		// in the last pass drops below some threshold of the whole picture size.
		// (This can still lead to infinite loops, if (for example) the plot
		// contains (almost) entirely a sub-set of the main set that's not
		// covered by the initial check. DDTT?)
		prev_livecount = livecount;
		livecount=0;
		for (i=0; i<NJOBS; i++) livecount += jobs[i].live_pixels;
		//printf("pass %d, max=%d, %u live pixels remain\n", passcount, this_pass_maxiter, livecount);
		if (livecount < width * height * (100-MIN_ESCAPEE_PCT) / 100) {
			unsigned delta = prev_livecount - livecount;
			if (delta < delta_threshold) {
				live = false;
				//printf("Threshold hit (only %d changed) - halting\n",prev_livecount - livecount);
			} else if (delta < 2*delta_threshold) {
				// This idea lifted from fanf's code.
				// Close enough unless it suddenly speeds up again next run?
				delta_threshold *= 2;
			}
		}

		if (callback && !_abort) {
			// How many pixels are live?
			ostringstream info;
			info << "Pass " << passcount << ": maxiter=" << this_pass_maxiter;
			string infos = info.str();
			callback->plot_progress_major(*this, this_pass_maxiter, infos);
		}

		last_pass_maxiter = this_pass_maxiter;
		if (passcount & 1) maxiter_scale = this_pass_maxiter / 2;
		this_pass_maxiter += maxiter_scale;
	} while (live && !_abort);

	plotted_maxiter = last_pass_maxiter;
	plotted_passes = passcount;
	if (!_abort) {
		// All done, anything that survived is considered to be infinite.
		for (i=0; i<height*width; i++) {
			if (_data[i].iter >= last_pass_maxiter)
				_data[i].iter = _data[i].iterf = -1;
		}
	}

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
	unsigned i, j, n_rows = job->n_rows;
	unsigned& live = job->live_pixels;

	// Sanity check: don't overrun the plot.
	if (firstrow + n_rows > height) {
		n_rows = height - firstrow;
		// Fix the initial livecount error.
		if (live > n_rows * width)
			live = n_rows * width;
	}
	const unsigned fencepost = firstrow + n_rows;

	unsigned out_index = firstrow * width;
	// keep running points.
	for (j=firstrow; j<fencepost; j++) {
		for (i=0; i<width; i++) {
			fractal_point& pt = _data[out_index];
			if (!pt.nomore) {
				fract->plot_pixel(job->maxiter, pt);
				if (pt.nomore)
					--live;
			}
			++out_index;
		}
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
cfpt Plot2::pixel_to_set(int x, int y)
{
	if (x<0) x=0; else if ((unsigned)x>width) x=width;
	if (y<0) y=0; else if ((unsigned)y>height) y=height;

	const fvalue pixwide = real(size) / width,
		  		 pixhigh  = imag(size) / height;
	cfpt delta (x*pixwide, y*pixhigh);
	return origin() + delta;
}

Plot2::~Plot2() {
	stop();
	wait();
	{
		Glib::Mutex::Lock _auto(plot_lock);
		delete[] _data;
		assert(_done);
		_data = 0; // Just in case concurrency runs awry.
	}
}
