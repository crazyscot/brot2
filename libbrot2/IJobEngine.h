/*
    IJobEngine.h: Interface for job processing engines
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

/*
 * A Job Engine has a pile of jobs.
 * They will be started in order but (of course) there is no guarantee of
 * completion order or finishing order.
 *
 * The Engine takes a callback, which is called both for single
 * jobs becoming complete, and for the whole thing.
 *
 * Some Engines may allow jobs to be added to (or even removed from) the
 * queue while the engine is running.
 */

#ifndef IJOBENGINE_H_
#define IJOBENGINE_H_

#include <vector>
#include "IJob.h"

class IJobEngine;

class IJobEngineCallback {
public:
	// A single Job has completed.
	virtual void JobComplete(IJob*, IJobEngine&) = 0;

	// Normal termination. All Jobs have finished.
	virtual void JobEngineFinished(IJobEngine&) = 0;

	// Somebody told us to stop. Called when all running Jobs have finished.
	virtual void JobEngineStopped(IJobEngine&) = 0;

	virtual ~IJobEngineCallback() {}
};

class IJobEngine {
public:
	/* NOTE: Implementations are expected to be passed an IJobEngineCallback
	 * and call it suitably. */

	/* Kicks off the engine.
	 * Most implementations will do this asynchronously i.e. return
	 * immediately from this call.
	 * They are expected to take care of their own threading if so. */
	virtual void start() = 0;

	/* Stops the queue. Will result in a JobEngineStopped callback when
	 * all running Jobs have stopped. */
	virtual void stop() = 0;

	virtual ~IJobEngine() {}
};

#endif /* IJOBENGINE_H_ */
