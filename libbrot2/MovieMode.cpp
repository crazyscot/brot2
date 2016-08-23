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

class NullCompletionHandler : public IMovieCompleteHandler {
	public:
		virtual void signal_completion(RenderJob&) {}
		virtual void signal_error(RenderJob&, const std::string&) {}
		virtual ~NullCompletionHandler() {}
};

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

};
