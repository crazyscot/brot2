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

#include <sigc++/sigc++.h>
#include "IJob.h"

namespace job {

class IJobEngine {
private:
	sigc::signal<void, IJob*> _signalJobDone;
	sigc::signal<void, IJob*> _signalJobFailed;
	sigc::signal<void> _signalFinished;
	sigc::signal<void> _signalStopped;
public:
	/* Kicks off the engine.
	 * Most implementations will do this asynchronously i.e. return
	 * immediately from this call.
	 * They are expected to take care of their own threading if so. */
	virtual void start() = 0;

	/* Stops the queue. Will result in a JobEngineStopped callback when
	 * all running Jobs have stopped. */
	virtual void stop() = 0;

	/* Note: The connect_*() methods return a connection, "which you can
	 * later use to disconnect your method. If the type of your object
	 * inherits from sigc::trackable the method is disconnected
	 * automatically when your object is destroyed."
	 */

	/* Who should we tell when a job is done? */
	sigc::connection connect_JobDone(const sigc::slot<void,IJob*>& slot) {
		return _signalJobDone.connect(slot);
	};
	/* Who should we tell when a job fails? */
	sigc::connection connect_JobFailed(const sigc::slot<void,IJob*>& slot) {
		return _signalJobFailed.connect(slot);
	};
	/* Who should we tell when all jobs are complete? */
	sigc::connection connect_Finished(const sigc::slot<void>& slot) {
		return _signalFinished.connect(slot);
	};
	/* Who should we tell when we've stopped? */
	sigc::connection connect_Stopped(const sigc::slot<void>& slot) {
		return _signalStopped.connect(slot);
	};

	/* NOTE: The engine will also emit individual jobs' signals as appropriate. */


	virtual ~IJobEngine() {}

protected:
	/* To be called by implementations when jobs complete or fail, as appropriate */
	void emit_JobDone(IJob* job) {
		_signalJobDone(job);
	}
	void emit_JobFailed(IJob* job) {
		_signalJobFailed(job);
	}
	/* To be called by implementations when finished or stopped, as appropriate */
	void emit_Finished() {
		_signalFinished.emit();
	}
	void emit_Stopped() {
		_signalStopped.emit();
	}
};

}; // ::job

#endif /* IJOBENGINE_H_ */
