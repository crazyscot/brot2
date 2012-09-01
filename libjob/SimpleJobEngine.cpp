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

job::SimpleJobEngine::SimpleJobEngine(const std::list<IJob*>& jobs) :
	_jobs(), _lock(), _halt(false)
{
	_jobs.assign(jobs.begin(), jobs.end());
}

void job::SimpleJobEngine::start()
{
	// Will be overridden in a more complicated subclass...
	run();
}

void job::SimpleJobEngine::run()
{
	std::list<IJob*>::iterator it;
	for (it=_jobs.begin(); it != _jobs.end() && !_halt; it++) {
		try {
			(*it)->run(*this);
		} catch (...) {
			emit_JobFailed(*it);
		}
		Glib::Mutex::Lock _auto (_lock);
		if (_halt) break;
	}
	Glib::Mutex::Lock _auto (_lock);
	if (_halt)
		emit_Stopped();
	else
		emit_Finished();
}

job::SimpleJobEngine::~SimpleJobEngine()
{
}

void job::SimpleJobEngine::stop()
{
	Glib::Mutex::Lock _auto (_lock);
	_halt = true;
}
