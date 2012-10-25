/*
    Plot3.cpp: Fractal plotting engine (third version)
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

#include <glibmm.h>
#include <unistd.h>
#include <values.h>
#include "Plot3.h"
#include "Prefs.h"
#include "ChunkDivider.h"

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

Plot3::Plot3(IPlot3DataSink* s, FractalImpl* f, Point centre, Point size,
		unsigned width, unsigned height, unsigned max_passes) :
		sink(s), fract(f), centre(centre), size(size),
		width(width), height(height),
		prefs(Prefs::getMaster()),
		//plotted_maxiter(0), plotted_passes(0),
		passes_max(max_passes),
		_engine(0)
		//callback(0), _data(0), _abort(false), _done(false), _outstanding(0),
		//_completed(0), jobs(0)
{
	if (passes_max==0)
		passes_max = (unsigned)-1;
}

string Plot3::info(bool verbose) const {
	std::ostringstream rv;
	/* LP#783087:
	 * Compute the size of a pixel in fractal units, then work out the
	 * decimal precision required to express that, plus 1 for a safety
	 * margin. */
	const Fractal::Value xpixsize = fabsl(real(size) / width),
						 ypixsize = fabsl(imag(size) / height);
	const int clampx = 1+ceill(0-log10(xpixsize)),
		      clampy = 1+ceill(0-log10(ypixsize));

	rv << fract->name << "@(";
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

/* Starts a plot. An IJobEngine is created to do the actual work. */
void Plot3::start(ChunkDivider::Base& factory) {
 	ASSERT(!_engine); // May not be null if a resume. Possibly need to delete first?

	factory.dividePlot(_chunks, sink, fract, centre, size, width, height, passes_max);

	std::list<Plot3Chunk*>::iterator it;
	for (it=_chunks.begin(); it != _chunks.end(); it++) {
		_jobs.push_back(*it);
	}

	_engine = new job::MultiThreadJobEngine(_jobs);
	_engine->start();
}

void Plot3::stop() {
	if (_engine)
		_engine->stop(true);
}

void Plot3::wait() {
	if (_engine) {
		_engine->wait();
		delete _engine;
		_engine = 0;
	}
}

Plot3::~Plot3() {
	stop();
	wait();
	//if (engine) delete engine;//implicit by wait()
	std::list<Plot3Chunk*>::iterator it;
	for (it=_chunks.begin(); it != _chunks.end(); it++) {
		delete *it;
	}
	// No need to free up _jobs.
}

#if 0
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

	initial_maxiter = prefs->get(PREF(InitialMaxIter));
	live_threshold = prefs->get(PREF(LiveThreshold));
	minimum_escapee_percent = prefs->get(PREF(MinEscapeePct));
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
	ASSERT((unsigned)y < height);
	ASSERT((unsigned)x < width);
	return _data[y * width + x];
}
#endif

void Plot3::set_prefs(std::shared_ptr<const Prefs>& newprefs) {
	prefs = newprefs;
}

void Plot3::set_prefs(std::shared_ptr<Prefs>& newprefs) {
	// We don't need to write to it...
	std::shared_ptr<const Prefs> p2(newprefs);
	prefs = p2;
}

unsigned Plot3::chunks_outstanding() const {
	return _jobs.size();
}

unsigned Plot3::chunks_total() const {
	return _chunks.size();
}

} // namespace Plot3
