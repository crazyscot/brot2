/*
    SimpleAsyncJobEngine.cpp: Asynchronous JobEngine.
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

#ifndef SIMPLEASYNCJOBENGINE_H_
#define SIMPLEASYNCJOBENGINE_H_

#include <glibmm/thread.h>
#include "SimpleJobEngine.h"

class SimpleAsyncJobEngine: public SimpleJobEngine {
	Glib::Thread* _thread;
	Glib::Mutex _lock; // Protects _thread
public:
	SimpleAsyncJobEngine(IJobEngineCallback& callback, std::list<IJob*>& jobs);
	virtual void start();
	virtual ~SimpleAsyncJobEngine();
	virtual void wait(); // Waits until all jobs completed
};

#endif /* SIMPLEASYNCJOBENGINE_H_ */
