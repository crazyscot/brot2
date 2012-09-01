/*
    IGlibSlottedJobEngine.h: Abstract job processing engine with Glib slots for signalling
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

#ifndef IGLIBSLOTTEDJOBENGINE_H_
#define IGLIBSLOTTEDJOBENGINE_H_

#include "IJobEngine.h"
#include <sigc++/sigc++.h>
#include <glibmm/thread.h>

namespace job {

class IGlibSlottedJobEngine: public job::IJobEngine {
public:
	/* Note: The connect_*() methods return a connection, "which you can
	 * later use to disconnect your method. If the type of your object
	 * inherits from sigc::trackable the method is disconnected
	 * automatically when your object is destroyed."
	 *
	 * If we should find we want to disconnect these slots, we'll need to
	 * implement a way to do so with the engine lock held.
	 */

	/* Who should we tell when a job fails? */
	void connect_JobFailed(const sigc::slot<void,IJob*>& slot) {
		Glib::Mutex::Lock _auto(_sigLock);
		_signalJobFailed.connect(slot);
	};
	/* Who should we tell when all jobs are complete? */
	void connect_Finished(const sigc::slot<void>& slot) {
		Glib::Mutex::Lock _auto(_sigLock);
		_signalFinished.connect(slot);
	};
	/* Who should we tell when we've stopped? */
	void connect_Stopped(const sigc::slot<void>& slot) {
		Glib::Mutex::Lock _auto(_sigLock);
		_signalStopped.connect(slot);
	};

protected:
	/* To be called by implementations */
	void emit_JobFailed(IJob* job) {
		Glib::Mutex::Lock _auto(_sigLock);
		_signalJobFailed(job);
	}
	void emit_Finished() {
		Glib::Mutex::Lock _auto(_sigLock);
		_signalFinished.emit();
	}
	void emit_Stopped() {
		Glib::Mutex::Lock _auto(_sigLock);
		_signalStopped.emit();
	}

private:
	/*
	 * Beware!
	 * http://answerpot.com/showthread.php?2326750-Threads+and+gtkmm says:
	 *
	 * 2. Unless special additional synchronization is employed, any
	 * particular sigc::signal object should be regarded as 'owned' by the
	 * thread which created it. Only that thread should connect slots with
	 * respect to the signal object, and only that thread should emit() or
	 * call operator()() on it.
	 *
	 * 3. Unless special additional synchronization is employed, any
	 * sigc::connection object should be regarded as 'owned' by the thread
	 * which created the slot and called the method which provided the
	 * sigc::connection object. Only that thread should call
	 * sigc::connection methods on the object.
	 *
	 *
	 * Also beware that signals are emitted WITH THE ENGINE LOCK HELD!
	 * This might lead to all manner of interesting deadlocks if the signals
	 * do anything nontrivial.
	 *
	 * In a complex multi-threaded environment it would be a good idea to use
	 * a Glib::Dispatcher or similar method to asynchronously pass these signal
	 * over to another thread to actually act on them.
	 */
	Glib::Mutex _sigLock; // Protects all signal slots
	sigc::signal<void, IJob*> _signalJobFailed;
	sigc::signal<void> _signalFinished;
	sigc::signal<void> _signalStopped;

};

}; // ::job

#endif /* IGLIBSLOTTEDJOBENGINE_H_ */
