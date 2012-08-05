/*
 * JobsTest.cpp
 *
 *  Created on: 5 Aug 2012
 *      Author: wry
 */

#include "gtest/gtest.h"
#include "SimpleJobEngine.h"
#include <list>

class TestingJob: public IJob {
public:
	TestingJob() : _hasrun (false) {}

	virtual void run(IJobEngine& engine);
	bool beenCalled() const { return _hasrun; }

	int serial() const { return _serial; }
	void serial(int i) { _serial = i; }

private:
	bool _hasrun;
	int _serial;
};

void TestingJob::run(IJobEngine&)
{
	_hasrun = true;
}

class TestCallback: public IJobEngineCallback {
	bool _finished;
	int _N;
	bool* _jobsfinished;
public:

	TestCallback(int N) : _finished(false), _N(N) {
		_jobsfinished = new bool[N];
		for (int i=0; i<N; i++) _jobsfinished[i]=false;
	}
	virtual ~TestCallback() {
		delete[] _jobsfinished;
	}

	void JobComplete(IJob* j, IJobEngine&) {
		TestingJob* tj = dynamic_cast<TestingJob*>(j);
		ASSERT_LT(tj->serial(), _N);
		_jobsfinished[tj->serial()] = true;
	};
	void JobEngineFinished(IJobEngine&) {
		_finished = true;
	};
	void JobEngineStopped(IJobEngine&) {};

	void checks() {
		EXPECT_TRUE(_finished);
		for (int i=0; i<_N; i++) EXPECT_TRUE(_jobsfinished[i]);
	}
};

TEST(jobs, allJobsRunAndCallback) {
	const int N=10;
	TestingJob* jobs = new TestingJob[N];
	TestCallback cb(N);
	std::list<IJob*> list;

	for (int i=0; i<N; i++) {
		jobs[i].serial(i);
		list.push_back(&jobs[i]);
	}

	SimpleJobEngine engine(cb, list);
	engine.start(); // synchronous, so won't return until it's done

	cb.checks();
	delete[] jobs;
}
