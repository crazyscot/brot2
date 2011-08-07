/*
    reporter.cpp: progress reporting for the command-line
    Copyright (C) 2011 Ross Younger

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

#include "reporter.h"
#include <iostream>
#include <stdlib.h>

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

Reporter::Reporter(int columns)
{
	ncolumns = columns;
	if (!ncolumns)
		ncolumns = 80;
}

void Reporter::plot_progress_minor(Plot2& UNUSED(plot), float workdone)
{
	if (ncolumns > 10) {
		int j, n=ncolumns * workdone;
		for (j=0; j<n; j++)
			std::cout << '.';
	} else {
		std::cout << int(workdone * 100) << '%';
	}
	std::cout << '\r';
}

void Reporter::plot_progress_major(Plot2& UNUSED(plot),
		unsigned UNUSED(current_maxiter),
		std::string& commentary)
{
	std::cout << std::endl << commentary << std::endl;
}

void Reporter::plot_progress_complete(Plot2& UNUSED(plot))
{
	std::cout << std::endl << "Complete!" << std::endl;
}

