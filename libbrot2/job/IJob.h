/*
    IJob.h: An abstract job to run on an IJobEngine
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

/*
 * Little is fixed about jobs at an abstract level. They are a bit like
 * threads in that regard; indeed, they might well be run as threads
 * on a multicore machine.
 */

#ifndef IJOB_H_
#define IJOB_H_

namespace job {

class IJobEngine;

class IJob {

public:
	/* Called by the IJobEngine to run this job.
	 * Obviously, this is the bit that concrete implementations need
	 * to provide. */
	virtual void run(IJobEngine& engine) = 0;

	virtual ~IJob() {}
};

}; // ::job

#endif /* IJOB_H_ */
