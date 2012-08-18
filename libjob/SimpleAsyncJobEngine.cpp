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

#include "SimpleAsyncJobEngine.h"

job::SimpleAsyncJobEngine::SimpleAsyncJobEngine(std::list<IJob*>& jobs) :
	SimpleJobEngine(jobs),
		_thread(0), _lock()
{
}

void job::SimpleAsyncJobEngine::start()
{
	Glib::Mutex::Lock _auto(_lock);
	_thread = Glib::Thread::create(sigc::mem_fun(*this, &SimpleAsyncJobEngine::run), true);
}

job::SimpleAsyncJobEngine::~SimpleAsyncJobEngine()
{
	Glib::Mutex::Lock _auto(_lock);
	if (_thread != NULL) {
		_thread->join();
		_thread = NULL;
	}
}

void job::SimpleAsyncJobEngine::wait()
{
	Glib::Mutex::Lock _auto(_lock);
	if (_thread != NULL) {
		_thread->join();
		_thread = NULL;
	}
}
