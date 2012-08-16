/*
 * JobsTest.cpp
 *
 *  Created on: 5 Aug 2012
 *      Author: wry
 */

#include "gtest/gtest.h"
#include "job/SimpleJobEngine.h"
#include "job/SimpleAsyncJobEngine.h"
#include "job/MultiThreadJobEngine.h"
#include <list>
#include <glibmm/thread.h>
#include <glibmm/timer.h>

using namespace job;

typedef testing::Types<SimpleAsyncJobEngine, MultiThreadJobEngine> asyncEngines;
typedef testing::Types<SimpleJobEngine, SimpleAsyncJobEngine> allEngines;

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

/* Unblocks a LockstepJob after a certain amount of time has passed. */
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

///////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////

class JobsRun: public ::testing::Test {
protected:
	const int N;
	JobsRun() : N(10) {}
	TestingJob* jobs;
	CompletionListener listener;
	std::list<IJob*> list;

	virtual void SetUp() {
		jobs = new TestingJob[N];

		for (int i=0; i<N; i++) {
			list.push_back(&jobs[i]);
		}
	}
	virtual void TearDown() {
		for (int i=0; i<N; i++) {
			jobs[i].checkHasRun();
			jobs[i].checkDone();
		}
		listener.checkFinished();
		delete[] jobs;
	}
};

TEST_F(JobsRun, Simple) {
	SimpleJobEngine engine(list);
	listener.attachTo(engine);
	engine.start(); // synchronous, so won't return until it's done
}

template<class T>
class JobsRun2: public JobsRun {
public:
	JobsRun2(): JobsRun() {};
};

TYPED_TEST_CASE(JobsRun2, asyncEngines);

TYPED_TEST(JobsRun2, asyncEngines) {
	TypeParam engine(this->list);
	this->listener.attachTo(engine);
	engine.start();
	engine.wait();
}

///////////////////////////////////////////////////////////////////////////

class JobsStop: public ::testing::Test {
protected:
	std::list<IJob*> list;
	LockstepJob jlock;
	TestingJob jord;
	CompletionListener listener;
	Unblocker unblocker;

	JobsStop() : unblocker(jlock,200) {}
	virtual void SetUp() {
		list.push_back(&jlock);
		list.push_back(&jord);
		jord.checkHasNotRun();
	}
	virtual void TearDown() {
		//jord.checkHasNotRun(); // Actually, we don't know (or care) whether or not jord has run. A parallelising engine may very well run it.
		listener.checkStopped();
	}
};

TEST_F(JobsStop,Simple) {
	SimpleJobEngine engine(list);
	listener.attachTo(engine);
	unblocker.unblockAsynch();
	engine.stop(); // A bit horrible, but current behaviour is to set the _halt flag
	engine.start(); // synchronous, so won't return until it's done
}

template<class T>
class JobsStop2: public JobsStop {
public:
	JobsStop2(): JobsStop() {};
};

TYPED_TEST_CASE(JobsStop2, asyncEngines);

TYPED_TEST(JobsStop2, asyncEngines) {
	TypeParam engine(this->list);
	this->listener.attachTo(engine);
	engine.start();
	this->unblocker.unblockAsynch();
	engine.stop();
}

///////////////////////////////////////////////////////////////////////////

template<class T>
class ExceptionsCaught: public ::testing::Test {
protected:
	const int N;
	ExceptionsCaught() : N(10) {}
	TestingJob* jobs;
	CompletionListener listener;
	FailingJob fj;
	std::list<IJob*> list;

	virtual void SetUp() {
		jobs = new TestingJob[N];

		for (int i=0; i<N; i++) {
			list.push_back(&jobs[i]);
			if (i == 5)
				list.push_back(&fj);
		}
	}
	virtual void TearDown() {
		for (int i=0; i<N; i++) {
			jobs[i].checkHasRun();
			jobs[i].checkDone();
		}
		fj.checkHasNotRun();
		fj.checkFailed();
		listener.checkFinished();
		delete[] jobs;
	}
};


TYPED_TEST_CASE(ExceptionsCaught, allEngines);
TYPED_TEST(ExceptionsCaught, allEngines) {
	TypeParam engine(this->list);
	this->listener.attachTo(engine);
	engine.start();
	this->listener.WaitUntilTermination();
}

///////////////////////////////////////////////////////////////////////////

