/*  Benchmarkable.cpp: Framework for benchmark suite
    Copyright (C) 2013 Ross Younger

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

#include "Benchmarkable.h"
#include <sys/time.h>

uint64_t Benchmarkable::benchmark() {
    struct timeval t_start, t_end;
    gettimeofday(&t_start, 0);
	run();
    gettimeofday(&t_end, 0);
    return 1000000 * (t_end.tv_sec - t_start.tv_sec) + t_end.tv_usec - t_start.tv_usec;
}
