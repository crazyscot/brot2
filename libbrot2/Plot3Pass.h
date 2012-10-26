/*
    Plot3Pass.h: One computation pass of a Plot.
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

#ifndef PLOT3PASS_H_
#define PLOT3PASS_H_

#include <list>
#include "libjob/ThreadPool.h"
#include "libbrot2/Plot3Chunk.h"

namespace Plot3 {

class Plot3Pass {
	/**
	 * A Pass is a single computation run through a Plot.
	 * Every point of every chunk is run until either it escapes or it hits the pass limit.
	 * Multiple chunks are run in parallel as far as possible via a threadpool.
	 *
	 * A Pass is responsible for notifying its client (usually a Plot3) when
	 * it has completed. Should an uncaught exception somehow happen in a
	 * chunk it will be propagated outwards.
	 *
	 * It is not possible to cleanly abort a Pass. You can destroy the
	 * threadpool, which will stop processing ASAP, but the unstarted futures
	 * will never be satisfied so the thread running the pass will block
	 * forever.
	 */
	ThreadPool& _pool;
	std::list<Plot3Chunk*>& _chunks;

public:
	Plot3Pass(ThreadPool& pool, std::list<Plot3Chunk*>& chunks);
	virtual ~Plot3Pass();

	/** Runs all the chunks, blocks until they are done. */
	void run();
};

}

#endif /* PLOT3PASS_H_ */
