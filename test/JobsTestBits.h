/*
    JobsTestBits.h: Handy stuff for the Jobs unit tests
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


#ifndef JOBSTESTBITS_H_
#define JOBSTESTBITS_H_

#include "gtest/gtest.h"
#include "job/SimpleJobEngine.h"
#include "job/SimpleAsyncJobEngine.h"
#include "job/MultiThreadJobEngine.h"

using namespace job;

typedef testing::Types<SimpleAsyncJobEngine, MultiThreadJobEngine> asyncEngines;
typedef testing::Types<SimpleJobEngine, SimpleAsyncJobEngine> allEngines;

// A simple job
class TestingJob: public IJob {
	friend class CompletionListener;
	bool _hasrun;
	int _serial;
	bool _done, _failed;

	void done(void) { _done = true; }
	void failed(void) { _failed = true; }
public:
	TestingJob() : _hasrun (false), _done(false), _failed(false) { }

	virtual void run(IJobEngine&) {
		_hasrun = true;
	}

	void checkHasRun() const { EXPECT_TRUE(_hasrun); }
	void checkHasNotRun() const { EXPECT_FALSE(_hasrun); }
	void checkDone() const   { EXPECT_TRUE(_done); EXPECT_FALSE(_failed); }
	void checkFailed() const { EXPECT_TRUE(_failed); EXPECT_FALSE(_done); }
};

/* A job that blocks until told to complete. */
class LockstepJob: public IJob {
	Glib::Mutex _lock;
	Glib::Cond _cond;
public:
	virtual void run(IJobEngine&) {
		Glib::Mutex::Lock lock (_lock);
		_cond.wait(_lock);
	}
	void unblock() {
		Glib::Mutex::Lock lock (_lock);
		_cond.broadcast();
	}
};

// Unblocks a LockstepJob after a certain amount of time has passed.
class Unblocker {
	LockstepJob& _j;
	unsigned _time; // ms
	Glib::Thread *_thread;
public:
	Unblocker(LockstepJob& j, unsigned millis) : _j(j), _time(millis), _thread(NULL) {}
	void unblockSynch() {
		Glib::usleep(1000*_time);
		_j.unblock();
	}
	void unblockAsynch() {
		_thread = Glib::Thread::create(sigc::mem_fun(*this, &Unblocker::unblockSynch), true);
	}
	virtual ~Unblocker() {
		if (_thread != NULL) {
			_thread->join();
			_thread = NULL;
		}
	}
};

// A job that fails
class FailingJob: public TestingJob {
public:
	virtual void run(IJobEngine&) {
		throw 42;
	}
};

////////////////////////////////////////////////////////////////////////

// Helper class that deals with the sigc signalling
class CompletionListener {
	bool _done, _stopped;
	Glib::Mutex _mux;
	Glib::Cond _cond; // Protected by mux

	void sigDone(void) {
		_done = true;
		Glib::Mutex::Lock lock(_mux);
		_cond.broadcast();
	}
	void sigStopped(void) {
		_stopped=true;
		Glib::Mutex::Lock lock(_mux);
		_cond.broadcast();
	}

	void jobDone(IJob* job) {
		TestingJob* tj = dynamic_cast<TestingJob*>(job);
		if (tj != NULL) {
			tj->done();
		}
	}
	void jobFailed(IJob* job) {
		TestingJob* tj = dynamic_cast<TestingJob*>(job);
		if (tj != NULL) {
			tj->failed();
		}
	}

public:
	CompletionListener() : _done(false), _stopped(false) {}

	void checkFinished(void) const {
		EXPECT_TRUE(_done);
		EXPECT_FALSE(_stopped);
	}
	void checkStopped(void) const {
		EXPECT_FALSE(_done);
		EXPECT_TRUE(_stopped);
	}

	void attachTo(IJobEngine& engine) {
		engine.connect_Finished(sigc::mem_fun(this, &CompletionListener::sigDone));
		engine.connect_Stopped(sigc::mem_fun(this, &CompletionListener::sigStopped));
		engine.connect_JobDone(sigc::mem_fun(this, &CompletionListener::jobDone));
		engine.connect_JobFailed(sigc::mem_fun(this, &CompletionListener::jobFailed));
	}

	void WaitUntilTermination() {
		Glib::Mutex::Lock lock(_mux);
		while (!_done && !_stopped)
			_cond.wait(_mux);
	}
};

#endif /* JOBSTESTBITS_H_ */
