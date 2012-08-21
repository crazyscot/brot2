/*
    MultiThreadJobEngine.cpp: A Glib::ThreadPool job engine.
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

#ifndef MULTITHREADJOBENGINE_H_
#define MULTITHREADJOBENGINE_H_

#include <list>
#include <glibmm/threadpool.h>
#include "SimpleJobEngine.h"

namespace job {

class MultiThreadJobEngine : public job::SimpleJobEngine {
public:
	MultiThreadJobEngine(std::list<IJob*>& jobs);
	virtual ~MultiThreadJobEngine();

	virtual void start();
	virtual void wait(); // Waits for all jobs to complete.

	/* Stops ASAP - no new jobs are started, then when they're all stopped we signal Stopped.
	 * If async=true, we return immediately.
	 * If async=false, we return after signalling Stopped.
	 */
	virtual void stop() { stop(false); };
	virtual void stop(bool async);

protected:
	Glib::ThreadPool threadpool;

	// Glib::Mutex _lock;
	// std::list<IJob*> _jobs; // Protect by _lock
	// std::atomic<bool> _halt; // Protect by _lock
	Glib::Cond _completion_beacon; // Protect by _lock
	int _livecount; // Protect by _lock
	bool _haveSignalledCompletion; // Protect by _lock
	Glib::Thread *_asyncCompletionThread;
	bool _haveShutDown; // Protect by _lock

	virtual void job_runner();
	virtual void done_runner();
	virtual void do_shutdown(bool asap);

};

};

#endif /* MULTITHREADJOBENGINE_H_ */
