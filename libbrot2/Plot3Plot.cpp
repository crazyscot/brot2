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

using namespace std;
using namespace Fractal;

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

Plot3Plot::Plot3Plot(IPlot3DataSink* s, FractalImpl& f, ChunkDivider::Base& d,
		Point centre, Point size,
		unsigned width, unsigned height, unsigned max_passes) :
		sink(s), fract(f), divider(d), runner(&Plot3Plot::threadfunc,this),
		centre(centre), size(size),
		width(width), height(height),
		prefs(Prefs::getMaster()),
		_shutdown(false),
		//plotted_maxiter(0), plotted_passes(0),
		passes_max(max_passes)
		//callback(0), _data(0), _abort(false), _done(false), _outstanding(0),
		//_completed(0), jobs(0)
{
	if (passes_max==0)
		passes_max = (unsigned)-1;
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
	rv << "???"; // FIXME //rv << plotted_maxiter;

	// Now that we autofix the aspect ratio, our pixels are square.
	double zoom = 1.0/real(size); // not a Value, so we can print it

	rv.precision(4); // Don't need more than this for the axis length or pixsize.
	if (verbose) {
		char buf[128];
		unsigned rr = snprintf(buf, sizeof buf, "%g", zoom);
		ASSERT(rr < sizeof buf);
		rv << ", zoom=" << buf;
		rv << " / axis length=" << size << " / pixel size=" << real(size)/width;
	} else {
		rv << " axis=" <<size;
	}

	return rv.str();
}

/* Starts a plot. The actual work happens in the background. */
void Plot3Plot::start() {
	divider.dividePlot(_chunks, sink, fract, centre, size, width, height, passes_max);
	post_message(Message::GO);
}

/** Sends a message to the worker thread */
void Plot3Plot::post_message(const Message& m) {
	// TODO: Is there really only GO? If so then reduce complexity...
	std::unique_lock<std::mutex> lock(_lock);
	_messages.push(m);
	_cond.notify_all();
}

/**
 * Our message-processing worker thread
 */
void Plot3Plot::threadfunc() {
	while(true) {
		Message m;
		{
			std::unique_lock<std::mutex> lock(_lock);
			while (_messages.empty()) {
				_cond.wait(lock);
				if (_shutdown) return;
			}
			m = _messages.front();
			_messages.pop();
		}
		switch(m) {
		case Message::GO:
			run();
			break;
		}
	}
}

/**
 * The actual work of running a Plot. This happens in its own thread.
 */
void Plot3Plot::run() {
	// HERE
	// - set up a Pass
	// - set up variables from P2 ?
	// - are we nearly there yet?
	// XXX check stop flag between passes
}

#if 0
void Plot3Plot::stop() {
	// XXX set stop flag - run() should check it
}

void Plot3Plot::wait() {
	// XXX
}
#endif

Plot3Plot::~Plot3Plot() {
	_shutdown = true;
	post_message(Message::GO);

	std::list<Plot3Chunk*>::iterator it;
	for (it=_chunks.begin(); it != _chunks.end(); it++) {
		delete *it;
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

/* TODO
unsigned Plot3Plot::chunks_outstanding() const {
	return _jobs.size();
}*/

unsigned Plot3Plot::chunks_total() const {
	return _chunks.size();
}

} // namespace Plot3
