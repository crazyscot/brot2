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
#include <unistd.h>
#include <values.h>
#include "Plot2.h"

using namespace std;
using namespace Fractal;

#define INITIAL_PASS_MAXITER 512
/* Judging the number of iterations and how to scale it on successive passes
 * is really tricky. Too many and it takes too long to get that first pass home;
 * too few (too slow growth) and you're wasting cycles keeping track of the
 * live count and your initial pass might be all-black anyway - not to mention
 * the chance of hitting the quality threshold sooner than you might have liked).
 * There are also edge-case effects where some pixels escape quickly and some
 * slowly - if the scaling is too slow then these plots will conk out too soon.
 */

/* We consider the plot to be complete when at least the minimum threshold
 * of the image has escaped, and the number of pixels escaping
 * in the last pass drops below some threshold of the whole picture size.
 * Beware that this can still lead to infinite loops if you look at regions
 * with excessive numbers of points within the set... */
#define LIVE_THRESHOLD_FRACT 0.001
#define MIN_ESCAPEE_PCT 20

class SingletonThreadPool {
	int n_threads, max_unused;
	const bool is_excl;
	Glib::StaticMutex mux;
	Glib::ThreadPool * pool;
public:
	// excl and max_idle are passed straight to Glib threadpool.
	// maxthreads may be an explicit count >0, -1 for unlimited, or
	// 0 to attempt to autodetect the number of CPUs on the system.
	SingletonThreadPool(bool excl, int maxthreads=0, int max_idle=1) : is_excl(excl) {
		reset_threadcounts(maxthreads, max_idle);
	};
	inline void reset_threadcounts(int max, int max_idle) {
		mux.lock();
		if (max==0)
			max = sysconf(_SC_NPROCESSORS_ONLN);
		if (max==0) 
			max=1; // Last-ditch default
		n_threads = max;

		max_unused = max_idle;
		if (pool) {
			pool->set_max_threads(n_threads);
			pool->set_max_unused_threads(max_unused);
		}
		mux.unlock();
	}
	inline int n_workers() const {
		return n_threads;
	}
	inline Glib::ThreadPool * get() {
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

// Pool for the main per-plot threads - these aren't so busy and
// there shouldn't be more than one around, so we aren't really
// restrictive with the settings.
static SingletonThreadPool per_plot_thread_pool(false, -1);

// Pool for the worker threads.
// Hard limit of outstanding jobs so we don't cane the CPU unnecessarily.
static SingletonThreadPool worker_thread_pool(true);

using namespace std;

Plot2::Plot2(FractalImpl* f, Point centre, Point size,
		unsigned width, unsigned height) :
		fract(f), centre(centre), size(size),
		width(width), height(height),
		plotted_maxiter(0), plotted_passes(0),
		callback(0), _data(0), _abort(false), _done(false), _outstanding(0),
		_completed(0), jobs(0)
{
}

string Plot2::info(bool verbose) const {
	std::ostringstream rv;
	rv.precision(MAXIMAL_DECIMAL_PRECISION);
	rv << fract->name
	   << "@(" << real(centre) << ", " << imag(centre) << ")";
	rv << ( verbose ? ", maxiter=" : " max=");
	rv << plotted_maxiter;

	// Now that we autofix the aspect ratio, our pixels are square.
	double zoom = 1.0/real(size); // not a Value, so we can print it

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
void Plot2::start(callback_t* c, bool is_resume) {
	Glib::Mutex::Lock _lock(plot_lock);
	assert(is_resume || !_done);
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
	void set(const unsigned max) {
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
	_data = new PointData[width * height];

	const Point _origin = origin(); // origin of the _whole plot_, not of firstrow
	unsigned i,j, out_index = 0;
	//std::cout << "render centre " << centre << "; size " << size << "; origin " << origin << std::endl;

	Point colstep = Point(real(size) / width,0);
	Point rowstep = Point(0, imag(size) / height);
	//std::cout << "rowstep " << rowstep << "; colstep "<<colstep << std::endl;
	Point render_point = _origin;

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
	Glib::Mutex::Lock _auto (plot_lock);
	unsigned i, passcount=plotted_passes, delta_threshold = width * height * LIVE_THRESHOLD_FRACT;
	unsigned joblines = (10000 / width + 1); // 10k points is a good number to do as a batch; round up to nearest line
	const unsigned NJOBS = (height < joblines) ? 1 : height / joblines;
	bool live = true;

	const unsigned step = (height + NJOBS - 1) / NJOBS;
	assert(step > 0);
	// Must round up to avoid a gap.

	if (!jobs) {
		prepare(); // Could push this into the job threads, if it were necessary.

		jobs = new worker_job[NJOBS];
		for (i=0; i<NJOBS; i++)
			jobs[i] = worker_job(this, step*i, step);
	}

	unsigned livecount=0, prev_livecount;
	for (i=0; i<NJOBS; i++) livecount += jobs[i].live_pixels;
#ifdef DEBUG_LIVECOUNT
	printf("Initial livecount %u\n", livecount);
#endif

	int this_pass_maxiter = INITIAL_PASS_MAXITER, last_pass_maxiter = plotted_maxiter, maxiter_scale = 0;

	int _t_maxo = worker_thread_pool.n_workers();
	if (_t_maxo==-1) _t_maxo = MAXINT;
	const unsigned max_outstanding = _t_maxo;

	if (last_pass_maxiter) {
		if (passcount & 1)
			maxiter_scale = last_pass_maxiter / 2;
		else
			maxiter_scale = last_pass_maxiter / 3;

		this_pass_maxiter = last_pass_maxiter + maxiter_scale;

		if (this_pass_maxiter >= (INT_MAX/2)) {
			live=false;
			// TODO Explain to the user why we're not doing any more.
		}
	}
	while (live && !_abort) {
		++passcount;
		assert(_outstanding==0);
		_completed=0;

		unsigned out_ptr = 0;
		for (i=0; i<NJOBS; i++)
			jobs[i].set(this_pass_maxiter);

		while (out_ptr < NJOBS && !_abort) {
			while (_outstanding < max_outstanding && out_ptr < NJOBS) {
				++_outstanding; // plot_lock is held.
				// TODO don't push a job if it has no live pixels?
				worker_thread_pool.get()->push(sigc::mem_fun(jobs[out_ptr++], &worker_job::run));
			}
			_worker_signal.wait(plot_lock); // Signals that a job has completed, or we're asked to abort.
			if (!_abort) {
				if (callback) {
					float fractx = (float)_completed/NJOBS;
					_auto.release();
					callback->plot_progress_minor(*this, fractx);
					_auto.acquire();
				}
			}
		};
		// Now wait for them to finish.
		while(_outstanding) {
			_worker_signal.wait(plot_lock);
			if (callback) {
				float fractx = (float)_completed/NJOBS;
				_auto.release();
				callback->plot_progress_minor(*this, fractx);
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
#ifdef DEBUG_LIVECOUNT
		printf("pass %d, max=%d:\n", passcount, this_pass_maxiter);
#endif
		for (i=0; i<NJOBS; i++) {
#ifdef DEBUG_LIVECOUNT
			printf("  - job %2d live=%u\n",i,jobs[i].live_pixels);
#endif
			livecount += jobs[i].live_pixels;
		}
#ifdef DEBUG_LIVECOUNT
		printf("total %u live pixels remain\n", livecount);
#endif
		if (livecount < width * height * (100-MIN_ESCAPEE_PCT) / 100) {
			unsigned delta = prev_livecount - livecount;
			if (delta < delta_threshold) {
				live = false;
#ifdef DEBUG_LIVECOUNT
				printf("Threshold hit (only %d changed) - halting\n",prev_livecount - livecount);
#endif
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
			info << ": " << livecount << " pixels live";
			string infos = info.str();
			_auto.release();
			callback->plot_progress_major(*this, this_pass_maxiter, infos);
			_auto.acquire();
		}

		last_pass_maxiter = this_pass_maxiter;
		if (passcount & 1) maxiter_scale = this_pass_maxiter / 2;
		this_pass_maxiter += maxiter_scale;
		if (this_pass_maxiter >= (INT_MAX/2)) live=false; // lest we overflow
	}

	plotted_maxiter = last_pass_maxiter;
	plotted_passes = passcount;
	if (!_abort) {
		// All done, anything that survived is considered to be infinite.
		for (i=0; i<height*width; i++) {
			if (_data[i].iter >= last_pass_maxiter)
				_data[i].iter = _data[i].iterf = -1;
			else if (_data[i].iterf < Fractal::PointData::ITERF_LOW_CLAMP)
				_data[i].iterf = Fractal::PointData::ITERF_LOW_CLAMP;
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
			PointData& pt = _data[out_index];
			if (!pt.nomore) {
				fract->plot_pixel(job->maxiter, pt);
				if (pt.nomore)
					--live;
			}
			++out_index;
		}
		if (_abort) break;
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
Point Plot2::pixel_to_set(int x, int y) const
{
	if (x<0) x=0; else if ((unsigned)x>width) x=width;
	if (y<0) y=0; else if ((unsigned)y>height) y=height;

	const Value pixwide = real(size) / width,
		  		 pixhigh  = imag(size) / height;
	Point delta (x*pixwide, y*pixhigh);
	return origin() + delta;
}

/* Returns data for a single point, identified by its pixel co-ordinates within the plot. Does NOT understand antialiasing. */
const Fractal::PointData& Plot2::get_pixel_point(int x, int y)
{
	Glib::Mutex::Lock _auto(plot_lock);
	assert((unsigned)y < height);
	assert((unsigned)x < width);
	return _data[y * width + x];
}


Plot2::~Plot2() {
	stop();
	wait();
	{
		Glib::Mutex::Lock _auto(plot_lock);
		delete[] _data;
		if (jobs) delete[] jobs;
		assert(_done);
		_data = 0; // Just in case concurrency runs awry.
	}
}
