/*
    SimpleJobEngine.h: The simplest possible synchronous JobEngine.
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

#ifndef SIMPLEJOBENGINE_H_
#define SIMPLEJOBENGINE_H_

#include <list>
#include <atomic>
#include <glibmm/thread.h>
#include "IJobEngine.h"

/* The simplest possible synchronous JobEngine.
 * Does not allow jobs to be added later. */

class SimpleJobEngine: public virtual IJobEngine {
public:
	SimpleJobEngine(IJobEngineCallback& callback, const std::list<IJob*>& jobs);
	virtual ~SimpleJobEngine();

	virtual void start();
	virtual void stop(); // Of limited use as this class is synchronous, but ought to work nevertheless.

protected:
	IJobEngineCallback& _callback;
	std::list<IJob*> _jobs;

	Glib::Mutex _lock;
	std::atomic<bool> _halt; // Protect by _lock

	virtual void run(); // Does the actual work of iterating through the jobs.
};

#endif /* SIMPLEJOBENGINE_H_ */
