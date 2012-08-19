/*
    JobsTest.cpp: Unit tests for the Jobs engines
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

#include "gtest/gtest.h"
#include "SimpleJobEngine.h"
#include "SimpleAsyncJobEngine.h"
#include "MultiThreadJobEngine.h"
#include <list>
#include <glibmm/thread.h>
#include <glibmm/timer.h>

#include "JobsTestBits.h"

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

	JobsStop() : unblocker(jlock,50) {}
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

template<class T>
class JobsStop1: public JobsStop {};

TYPED_TEST_CASE(JobsStop1, syncEngines);

TYPED_TEST(JobsStop1, syncEngines) {
	TypeParam engine(this->list);
	this->listener.attachTo(engine);
	this->unblocker.unblockAsynch();
	engine.stop(); // A bit horrible, but current behaviour is to set the _halt flag
	engine.start(); // synchronous, so won't return until it's done
}

template<class T>
class JobsStop2: public JobsStop {};

TYPED_TEST_CASE(JobsStop2, asyncEngines);

TYPED_TEST(JobsStop2, asyncEngines) {
	TypeParam engine(this->list);
	this->listener.attachTo(engine);
	engine.start();
	this->unblocker.unblockAsynch();
	engine.stop();
}

TEST_F(JobsStop, MTJE_Async) {
	MultiThreadJobEngine engine(list);
	listener.attachTo(engine);
	engine.start();
	engine.stop(true);
	// We could optionally wait a bit here to demonstrate that it's not finishing in any short time.
	listener.checkLive();
	unblocker.unblockSynch();
	listener.WaitUntilTermination(); // Without this we sometimes fail, showing the asynch is indeed happening asynchly
	listener.checkStopped();
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

