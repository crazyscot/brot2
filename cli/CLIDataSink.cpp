/*
    CLIDataSink.cpp: progress reporting for the command-line
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

#include "CLIDataSink.h"
#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

#include "Exception.h"

using namespace Plot3;

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

CLIDataSink::CLIDataSink(int columns, bool silent) :
	ncolumns(columns), quiet(silent), _plot(0), _chunks_this_pass(0)
{
	if (!ncolumns) {
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		ncolumns = w.ws_col;
	}
	// Final fallback...
	if (!ncolumns)
		ncolumns = 80;
}

void CLIDataSink::chunk_done(Plot3Chunk* job)
{
	_chunks_done.insert(job);
	_chunks_this_pass++;

	if (quiet) return;
	float workdone = (float)_chunks_this_pass / _plot->chunks_total();
	// Without this lock, multiple threads bicker over the screen and make a mess.
	std::unique_lock<std::mutex> lock(_terminal_lock);
	ASSERT(workdone <= 1.0);
	if (ncolumns > 10) {
		int j, n=ncolumns * workdone;
		for (j=0; j<n; j++)
			std::cerr << '.';
	} else {
		std::cerr << int(workdone * 100) << '%';
	}
	std::cerr << '\r';
}

void CLIDataSink::pass_complete(std::string& commentary)
{
	_chunks_this_pass=0;
	if (quiet) return;
	std::cerr << std::endl << commentary << std::endl;
}

void CLIDataSink::plot_complete()
{
}
