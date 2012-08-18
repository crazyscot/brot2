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

#include "MultiThreadJobEngine.h"
#include <unistd.h>

namespace job {

MultiThreadJobEngine::MultiThreadJobEngine(std::list<IJob*>& jobs) :
		job::SimpleJobEngine(jobs), _livecount(0),
		_haveSignalledCompletion(false), _asyncCompletionThread(0)

{
	threadpool.set_max_threads(0);
}

MultiThreadJobEngine::~MultiThreadJobEngine() {
	stop();
	if (_asyncCompletionThread) {
		_asyncCompletionThread->join();
		_asyncCompletionThread = NULL;
	}
}

void MultiThreadJobEngine::start()
{
	Glib::Mutex::Lock _auto (_lock);

	// @TODO Ports to other OSes will need different code here.
	// See http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
	long cores = sysconf(_SC_NPROCESSORS_ONLN);
	threadpool.set_max_threads(cores);

	std::list<IJob*>::iterator it;
	for (it=_jobs.begin(); it != _jobs.end(); it++)
		threadpool.push(sigc::mem_fun(this, &MultiThreadJobEngine::job_runner));
	// And one more to mark the end.
	threadpool.push(sigc::mem_fun(this, &MultiThreadJobEngine::done_runner));
}

void MultiThreadJobEngine::job_runner()
{
	IJob *job;
	{
		Glib::Mutex::Lock _auto (_lock);
		job = _jobs.front();
		_jobs.pop_front();
		++_livecount;
	}
	try {
		job->run(*this);
		emit_JobDone(job);
	} catch (...) {
		emit_JobFailed(job);
	}
	{
		Glib::Mutex::Lock _auto (_lock);
		--_livecount;
		_completion_beacon.broadcast();
	}
}

void MultiThreadJobEngine::done_runner()
{
	Glib::Mutex::Lock _auto (_lock);
	while (_livecount) {
		_completion_beacon.wait(_lock);
	}
	if (!_haveSignalledCompletion) {
		if (_halt)
			emit_Stopped();
		else
			emit_Finished();
		_haveSignalledCompletion=true;
	}
}

void MultiThreadJobEngine::wait()
{
	threadpool.shutdown(false);
}

void MultiThreadJobEngine::do_shutdown()
{
	threadpool.shutdown(true);
	done_runner();
}

void MultiThreadJobEngine::stop(bool async)
{
	{
		Glib::Mutex::Lock _auto (_lock);
		_halt = true;
	}
	if (async) {
		if (!_asyncCompletionThread)
			_asyncCompletionThread = Glib::Thread::create(sigc::mem_fun(this, &MultiThreadJobEngine::do_shutdown), true);
		// else already shutting down, ignore
	} else {
		do_shutdown();
	}
}


};
