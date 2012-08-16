/*
 * JobsTest.cpp
 *
 *  Created on: 5 Aug 2012
 *      Author: wry
 */

#include "gtest/gtest.h"
#include "job/SimpleJobEngine.h"
#include "job/SimpleAsyncJobEngine.h"
#include <list>
#include <glibmm/thread.h>
#include <glibmm/timer.h>

using namespace job;

typedef testing::Types<SimpleAsyncJobEngine> asyncEngines;
typedef testing::Types<SimpleJobEngine, SimpleAsyncJobEngine> allEngines;

class TestingJob: public IJob {
	bool _hasrun;
	int _serial;
public:
	TestingJob() : _hasrun (false), _serial(-1) {}

	virtual void run(IJobEngine&) {
		_hasrun = true;
	}
	bool beenCalled() const { return _hasrun; }

	int serial() const { return _serial; }
	void serial(int i) { _serial = i; }
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
	bool _failcaught;
public:
	virtual void run(IJobEngine&) {
		throw 42;
	}
	bool wasCaught() const { return _failcaught; }
};

///////////////////////////////////////////////////////////////////////////

/* Simple checking callback */
class TestCallback: public IJobEngineCallback {
	bool _finished, _stopped;
	int _N;
	bool* _jobsfinished;
	bool* _jobsfailed;

	Glib::Mutex _mux;
	Glib::Cond _cond; // Protected by mux

public:
	TestCallback(int N) : _finished(false), _stopped(false), _N(N) {
		_jobsfinished = new bool[N];
		_jobsfailed = new bool[N];
		for (int i=0; i<N; i++) {
			_jobsfinished[i]=false;
			_jobsfailed[i]=false;
		}
	}
	virtual ~TestCallback() {
		delete[] _jobsfinished;
		delete[] _jobsfailed;
	}

	void JobComplete(IJob* j, IJobEngine&) {
		TestingJob* tj = dynamic_cast<TestingJob*>(j);
		if (tj != NULL) {
			ASSERT_LT(tj->serial(), _N);
			/* A test may not care that its jobs have signalled completion,
			 * in which case the default serial number -1 is ignored. */
			if (tj->serial() >= 0)
				_jobsfinished[tj->serial()] = true;
		}
	};
	void JobFailed(IJob* j, IJobEngine&) {
		FailingJob* tj = dynamic_cast<FailingJob*>(j);
		ASSERT_TRUE(tj);
		if (tj == NULL) {
			FAIL(); // Unexpected exception
		} else {
			ASSERT_LT(tj->serial(), _N);
			if (tj->serial() >= 0)
				_jobsfailed[tj->serial()] = true;
		}
	}
	void JobEngineFinished(IJobEngine&) {
		_finished = true;
		Glib::Mutex::Lock lock(_mux);
		_cond.broadcast();
	};
	void JobEngineStopped(IJobEngine&) {
		_stopped = true;
		Glib::Mutex::Lock lock(_mux);
		_cond.broadcast();
	};

	void WaitUntilTermination() {
		Glib::Mutex::Lock lock(_mux);
		while (!_finished && !_stopped)
			_cond.wait(_mux);
	}

	void checkAllDone() {
		checkFinished();
		for (int i=0; i<_N; i++) EXPECT_TRUE(_jobsfinished[i]);
	}
	void checkAllDoneExcept(int failedOne) {
		checkFinished();
		for (int i=0; i<_N; i++) {
			if (i==failedOne) {
				EXPECT_FALSE(_jobsfinished[i]);
				EXPECT_TRUE(_jobsfailed[i]);
			} else {
				EXPECT_TRUE(_jobsfinished[i]);
				EXPECT_FALSE(_jobsfailed[i]);
			}
		}
	}

	void checkFinished() {
		EXPECT_TRUE(_finished);
		EXPECT_FALSE(_stopped);
	}
	void checkStopped() {
		EXPECT_FALSE(_finished);
		EXPECT_TRUE(_stopped);
	}
};

///////////////////////////////////////////////////////////////////////////

class JobsRun: public ::testing::Test {
protected:
	const int N;
	JobsRun() : N(10) {}
	TestingJob* jobs;
	TestCallback* cb;
	std::list<IJob*> list;

	virtual void SetUp() {
		jobs = new TestingJob[N];
		cb = new TestCallback(N);

		for (int i=0; i<N; i++) {
			jobs[i].serial(i);
			list.push_back(&jobs[i]);
		}
	}
	virtual void TearDown() {
		cb->checkAllDone();
		delete[] jobs;
		delete cb;
	}
};

TEST_F(JobsRun, Simple) {
	SimpleJobEngine engine(*cb, list);
	engine.start(); // synchronous, so won't return until it's done
}

template<class T>
class JobsRun2: public JobsRun {
public:
	JobsRun2(): JobsRun() {};
};

TYPED_TEST_CASE(JobsRun2, asyncEngines);

TYPED_TEST(JobsRun2, asyncEngines) {
	TypeParam engine(*(this->cb), this->list);
	engine.start();
	engine.wait();
}

///////////////////////////////////////////////////////////////////////////

class JobsStop: public ::testing::Test {
protected:
	std::list<IJob*> list;
	LockstepJob jlock;
	TestingJob jord;
	TestCallback* cb;
	Unblocker* unblocker;
	virtual void SetUp() {
		cb = new TestCallback(1);
		unblocker = new Unblocker(jlock,100);
		list.push_back(&jlock);
		list.push_back(&jord);
		EXPECT_FALSE(jord.beenCalled());
	}
	virtual void TearDown() {
		EXPECT_FALSE(jord.beenCalled());
		cb->checkStopped();
		delete cb;
		delete unblocker;
	}
};

TEST_F(JobsStop,Simple) {
	SimpleJobEngine engine(*cb, list);
	unblocker->unblockAsynch();
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
	TypeParam engine(*(this->cb), this->list);
	engine.start();
	engine.stop();
	this->unblocker->unblockSynch();
}

///////////////////////////////////////////////////////////////////////////

template<class T>
class ExceptionsCaught: public ::testing::Test {
protected:
	const int N;
	ExceptionsCaught() : N(10) {}
	TestingJob* jobs;
	TestCallback* cb;
	FailingJob* fj;
	std::list<IJob*> list;

	virtual void SetUp() {
		jobs = new TestingJob[N];
		cb = new TestCallback(N);
		fj = new FailingJob();
		fj->serial(5);

		for (int i=0; i<N; i++) {
			if (i == 5) {
				list.push_back(fj);
			} else {
				jobs[i].serial(i);
				list.push_back(&jobs[i]);
			}
		}
	}
	virtual void TearDown() {
		cb->checkAllDoneExcept(5);
		delete[] jobs;
		delete cb;
		delete fj;
	}
};


TYPED_TEST_CASE(ExceptionsCaught, allEngines);
TYPED_TEST(ExceptionsCaught, allEngines) {
	TypeParam engine(*(this->cb), this->list);
	engine.start();
	this->cb->WaitUntilTermination();
}
