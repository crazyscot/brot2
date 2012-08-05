/*
    SimpleJobEngine.cpp: The simplest possible synchronous JobEngine.
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

#include "SimpleJobEngine.h"

SimpleJobEngine::SimpleJobEngine(IJobEngineCallback & callback,
		std::list<IJob*>& jobs) : _callback(callback)
{
	_jobs.assign(jobs.begin(), jobs.end());
	_halt = false;
}

void SimpleJobEngine::start()
{
	// Will be overridden in a more complicated subclass...
	run();
}

void SimpleJobEngine::run()
{
	std::list<IJob*>::iterator it;
	for (it=_jobs.begin(); it != _jobs.end() && !_halt; it++) {
		(*it)->run(*this);
		_callback.JobComplete(*it, *this);
		Glib::Mutex::Lock _auto (_lock);
		if (_halt) break;
	}
	Glib::Mutex::Lock _auto (_lock);
	if (_halt)
		_callback.JobEngineStopped(*this);
	else
		_callback.JobEngineFinished(*this);
}

SimpleJobEngine::~SimpleJobEngine()
{
}

void SimpleJobEngine::stop()
{
	Glib::Mutex::Lock _auto (_lock);
	_halt = true;
}