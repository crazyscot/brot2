/*
    MovieMode: brot2 movie plotting
    Copyright (C) 2016 Ross Younger

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

#include "MovieMode.h"
#include "MovieRender.h"
#include "IMovieProgress.h"
#include "ThreadPool.h"

using namespace Movie;

namespace Movie {

std::shared_ptr<ThreadPool> MovieInfo::movieinfo_runner_thread(new ThreadPool(1));

// How many frames is this movie? Runs the actual code but with a null renderer to determine.
unsigned MovieInfo::count_frames() const {
	NullRenderer renderer;
	MovieNullProgress progress;
	NullCompletionHandler completion;
	std::shared_ptr<const BrotPrefs::Prefs> prefs(BrotPrefs::Prefs::getMaster());

	try {
		renderer.do_blocking(progress, completion, "" /*filename*/, *this, prefs, movieinfo_runner_thread, "dummy-argv0");
	} catch (FrameLimitExceeded) {
	} catch (std::exception &e) {
		std::cerr << "FATAL: Uncaught exception in renderer: " << e.what() << std::endl << std::endl;
		exit(5);
	}
	return renderer.framecount();
}

double KeyFrame::zoom() const {
	// TODO: Largest complex viewport on any current fractal is 8x6; most are 6x6. We'll take 6 units as zoom 1.0. This may need to be changed later.
	return 6.0 / real(size);
}

double KeyFrame::logzoom() const {
	return log(zoom());
}

KeyFrame Vector::apply(const KeyFrame& _base) const {
	Fractal::Value xout, yout, zout;
	// X,Y easy
	xout = real(_base.centre) + x;
	yout = imag(_base.centre) + y;
	// Z a bit harder
	zout = 6.0 / exp( _base.logzoom() + z );
	double aspect = real(_base.size) / imag(_base.size);
	return KeyFrame(xout, yout, zout, zout / aspect, _base.speed_zoom, _base.speed_translate, _base.ease_in, _base.ease_out);
}

Vector::Vector(const struct KeyFrame& f1, const struct KeyFrame& f2) {
	x = real(f2.centre) - real(f1.centre);
	y = imag(f2.centre) - imag(f1.centre);
	z = f2.logzoom() - f1.logzoom();
}
};
