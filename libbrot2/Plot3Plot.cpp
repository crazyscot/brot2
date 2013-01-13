/*
    Plot3Plot.cpp: Fractal plotting engine (third version)
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

#include <unistd.h>
#include <values.h>
#include "Plot3Plot.h"
#include "Prefs.h"
#include "ChunkDivider.h"
#include <thread>
#include "libbrot2/ThreadPool.h"

using namespace std;
using namespace Fractal;
using namespace BrotPrefs;

#if 0
#define DEBUG_LIVECOUNT(x) x
#else
#define DEBUG_LIVECOUNT(x)
#endif

/* Judging the number of iterations and how to scale it on successive passes
 * is really tricky. Too many and it takes too long to get that first pass home;
 * too few (too slow growth) and you're wasting cycles keeping track of the
 * live count and your initial pass might be all-black anyway - not to mention
 * the chance of hitting the quality threshold sooner than you might have liked.
 * There are also edge-case effects where some pixels escape quickly and some
 * slowly - if the scaling is too slow then these plots will conk out too soon.
 */
//#define INITIAL_PASS_MAXITER 512 // Now read from Prefs

/* We consider the plot to be complete when at least the minimum threshold
 * of the image has escaped, and the number of pixels escaping
 * in the last pass drops below some threshold of the whole picture size.
 * Beware that this can still lead to infinite loops if you look at regions
 * with excessive numbers of points within the set... */
//#define LIVE_THRESHOLD_FRACT 0.001 // Now read from Prefs
//#define MIN_ESCAPEE_PCT 20 // Now read from Prefs

using namespace std;

namespace Plot3 {

Plot3Plot::Plot3Plot(ThreadPool& pool, IPlot3DataSink* s, FractalImpl& f, ChunkDivider::Base& d,
		Point centre, Point size,
		unsigned width, unsigned height, unsigned max_passes) :
		_pool(pool), sink(s), fract(f), divider(d),
		centre(centre), size(size),
		width(width), height(height),
		prefs(Prefs::getMaster()),
		_shutdown(false), _running(false), _stop(false),
		plotted_maxiter(0), plotted_passes(0),
		passes_max(max_passes),
		_runs_sema(0),
		runner(&Plot3Plot::threadfunc,this)
		// Note: Initialisation order is crucial when the threadfunc will immediately lock _lock !
		//callback(0), _data(0), _abort(false), _done(false), _outstanding(0),
		//_completed(0), jobs(0)
{
	if (passes_max==0)
		passes_max = (unsigned)-1;
}

Fractal::Value Plot3Plot::axis_to_zoom(Fractal::Value ax) const {
	return (fract.xmax - fract.xmin) / ax;
}
Fractal::Value Plot3Plot::zoom() const {
	return axis_to_zoom(real(size));
}

string Plot3Plot::info(bool verbose) const {
	std::ostringstream rv;
	/* LP#783087:
	 * Compute the size of a pixel in fractal units, then work out the
	 * decimal precision required to express that, plus 1 for a safety
	 * margin. */
	const Fractal::Value xpixsize = fabsl(real(size) / width),
						 ypixsize = fabsl(imag(size) / height);
	const int clampx = 1+ceill(0-log10(xpixsize)),
		      clampy = 1+ceill(0-log10(ypixsize));

	rv << fract.name << "@(";
	rv.precision(clampx);
	rv << real(centre) << ", ";
	rv.precision(clampy);
	rv << imag(centre) << ")";
	rv << ( verbose ? ", maxiter=" : " max=");
	rv << plotted_maxiter;

	// Now that we autofix the aspect ratio, our pixels are square.

	rv.precision(4); // Don't need more than this for the axis length or pixsize.
	if (verbose) {
		rv.precision(2);
		rv << ", zoom=" << scientific << zoom();
		rv.unsetf(ios::floatfield);
		rv << " / axis length=" << size << " / pixel size=" << real(size)/width;
	} else {
		rv << " axis=" <<size;
	}

	return rv.str();
}

string Plot3Plot::info_zoom(bool show_legend) const {
	/* In support of LP#1039385 */
	Fractal::Value z = zoom();
	std::ostringstream rv;
	if (show_legend)
		rv << "Zoom: ";
	else
		rv << "x";

	if (z > 10000.0) {
		rv.precision(2);
		rv << scientific;
		rv << z;
	} else {
		rv.precision(4);
		rv << z;
	}

	string rs = rv.str();
	if (z > 10000.0) {
		// Messy...
		size_t pos = rs.find("e");
		if(pos != std::string::npos)
			rs.replace(pos, 1, "x10<sup>");
		pos = rs.find("+0"); // Pesky extra zeroes, can't see how to turn them off
		if(pos != std::string::npos)
			rs.replace(pos, 2, "");
		pos = rs.find("+"); // This one shouldn't get triggered, but be paranoid
		if(pos != std::string::npos)
			rs.replace(pos, 1, "");
		rs.append("</sup>");
	}
	return rs;
}

/* Starts a plot. The actual work happens in the background. */
void Plot3Plot::start() {
	divider.dividePlot(_chunks, sink, fract, centre, size, width, height, passes_max);
	std::unique_lock<std::mutex> lock(_lock);
	_running = true;
	_stop = false;
	lock.unlock();
	poke_runner();
}

/* Asynch stop, return immediately */
void Plot3Plot::stop() {
	std::unique_lock<std::mutex> lock(_lock);
	_stop = true; // We'll notify when we've actually stopped.
}

/* Wait for completion. Does not call stop() first. */
void Plot3Plot::wait() {
	std::unique_lock<std::mutex> lock(_lock);
	if (!_running) return;
	_completion_cond.wait(lock);
}


/** Sends a message to the worker thread */
void Plot3Plot::poke_runner() {
	std::unique_lock<std::mutex> lock(_lock);
	++_runs_sema;
	_runs_cond.notify_all();
}

/**
 * Our message-processing worker thread
 */
void Plot3Plot::threadfunc() {
	std::unique_lock<std::mutex> lock(_lock);
	while(!_shutdown) {
		{
			while (!_runs_sema) {
				_runs_cond.wait(lock);
				if (_shutdown) return;
			}
			--_runs_sema;
		}
		lock.unlock();
		run();
		lock.lock();
	}
}

/**
 * The actual work of running a Plot. This happens in its own thread.
 */
void Plot3Plot::run() {
	std::unique_lock<std::mutex> lock(_lock);

	Plot3Pass pass(_pool, _chunks);
	unsigned live_pixels = width * height, live_pixels_prev;
	float live_threshold = prefs->get(PREF(LiveThreshold));
	unsigned minimum_escapee_percent = prefs->get(PREF(MinEscapeePct));
	unsigned delta_threshold = width * height * live_threshold;

	unsigned this_pass_maxiter = prefs->get(PREF(InitialMaxIter)),
			last_pass_maxiter = plotted_maxiter,
			maxiter_scale = 0,
			passcount = plotted_passes;

	_running = true;

	if (last_pass_maxiter) {
		if (passcount&1)
			maxiter_scale = last_pass_maxiter/2;
		else
			maxiter_scale = last_pass_maxiter/3;
		this_pass_maxiter = last_pass_maxiter + maxiter_scale;
		if (this_pass_maxiter >= (INT_MAX/2))
			_stop=true;
	} else {
		maxiter_scale = this_pass_maxiter;
	}

	while (!_stop & !_shutdown) {
		for (auto chunk : _chunks)
			chunk->reset_max_passes(this_pass_maxiter);

		lock.unlock();
		pass.run();
		lock.lock();
		DEBUG_LIVECOUNT(cout << "pass " << passcount << ", maxiter=" << this_pass_maxiter << endl );

		live_pixels_prev = live_pixels;
		live_pixels = 0;
		for (auto chunk : _chunks)
			live_pixels += chunk->livecount();
		unsigned pixel_threshold = width * height * (100-minimum_escapee_percent) / 100;
		DEBUG_LIVECOUNT(printf("total %u live pixels remain, threshold=%u\n", live_pixels, pixel_threshold));
		if (live_pixels==0) {
			_stop = true;
			DEBUG_LIVECOUNT(printf("No live pixels left - all done!\n"));
		} else if (live_pixels < pixel_threshold) {
			unsigned delta = live_pixels_prev - live_pixels;
			if (delta < delta_threshold) {
				_stop = true;
				DEBUG_LIVECOUNT(printf("Threshold hit (only %d changed) - halting\n",live_pixels_prev - live_pixels));
			} else if (delta < 2*delta_threshold) {
				// This idea lifted from fanf's code.
				// Close enough unless it suddenly speeds up again next run?
				delta_threshold *= 2;
				DEBUG_LIVECOUNT(std::cout << "Delta " << delta << " getting close to threshold " << delta_threshold << " - doubling threshold" << std::endl);
			} else {
				DEBUG_LIVECOUNT(std::cout << "Delta " << delta << " < threshold " << delta_threshold << " - still going" << std::endl);
			}
		}

		plotted_passes = ++passcount;
		plotted_maxiter = last_pass_maxiter = this_pass_maxiter;

		{
			// Notify this pass is complete
			ostringstream info;
			info << passcount << " pass" << (passcount==1 ? "" : "es") << " plotted: maxiter=" << plotted_maxiter;
			info << ": " << live_pixels << " pixels live";
			string infos = info.str();
			lock.unlock();
			sink->pass_complete(infos);
			lock.lock();
		}

		// Now set up for next pass
		if (passcount & 1) maxiter_scale = this_pass_maxiter / 2;
		if (maxiter_scale<1) maxiter_scale=1;
		this_pass_maxiter += maxiter_scale;
		if (this_pass_maxiter >= (INT_MAX/2)) _stop=true; // lest we overflow
	}

	// Any pixel still alive is considered to be infinite.
	// P3Chunk ensures that the point data is set up correctly for this.

	_running = false;
	lock.unlock();
	sink->plot_complete();
	lock.lock();
	_completion_cond.notify_all();
}

Plot3Plot::~Plot3Plot() {
	std::unique_lock<std::mutex> lock(_lock);
	_shutdown = true;
	lock.unlock();
	poke_runner();
	runner.join();
	for (auto it: _chunks) {
		delete it;
	}
}

void Plot3Plot::set_prefs(std::shared_ptr<const Prefs>& newprefs) {
	prefs = newprefs;
}

void Plot3Plot::set_prefs(std::shared_ptr<Prefs>& newprefs) {
	// We don't need to write to it...
	std::shared_ptr<const Prefs> p2(newprefs);
	prefs = p2;
}

unsigned Plot3Plot::chunks_total() const {
	return _chunks.size();
}

const std::list<Plot3Chunk*>& Plot3Plot::get_chunks__only_after_completion()
{
	wait();
	return _chunks;
}

bool Plot3Plot::is_running() {
	std::unique_lock<std::mutex> lock(_lock);
	return _running;
}

/* Converts an (x,y) pair on the render (say, from a mouse click) to their complex co-ordinates */
Point Plot3Plot::pixel_to_set_blo(int x, int y) const
{
	if (x<0) x=0; else if ((unsigned)x>width) x=width;
	if (y<0) y=0; else if ((unsigned)y>height) y=height;

	const Value pixwide = real(size) / width,
				pixhigh  = imag(size) / height;
	Point delta (x*pixwide, y*pixhigh);
	return origin() + delta;
}

} // namespace Plot3
