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

#ifndef BENCHMARKABLE_H_
#define BENCHMARKABLE_H_

#include <stdint.h>
#include <string>
#include <iostream>
#include <iomanip>

class Benchmarkable
{
public:
	Benchmarkable() {}

	// Actually runs the benchmark.
	// Returns the runtime in microseconds.
	// NOTE: When committing statistics you should pay attention to n_iterations().
	uint64_t benchmark();
	// How many iterations did we benchmark?
	virtual uint64_t n_iterations() = 0;
protected:
	// Code to be benchmarked
	virtual void run() = 0;

	virtual ~Benchmarkable() {}
};

const std::string YELLOW("\033[0;33m");
const std::string DEFAULT("\033[m");

struct stats {
	std::string title;
	uint64_t time; /* microseconds */
	uint64_t n_iters;

	stats(const std::string& _title, uint64_t _time /* us */, uint64_t _n_iters) :
		title(_title), time(_time), n_iters(_n_iters) {}

	void output() const {
		uint64_t sec = time / 1000000, usec = time % 1000000;
		long double mean_ps = time * 1000000 / n_iters;
		unsigned mean_ns = mean_ps / 1000,
				 mean_ns_fract = mean_ns % 1000;

		std::cout
			 << std::resetiosflags(std::ios::showbase)
		     << YELLOW
			 << "Benchmark:               " << title << std::endl
		     << DEFAULT
		     << std::setw(3)
		     << "Total time:              " << sec << '.' << std::setfill('0') << std::setw(6) << usec << 's' << std::endl
		     << std::resetiosflags(std::ios::showbase)
		     << "Number of iterations:    " << n_iters << std::endl
		     << "Mean time per iteration: " << mean_ns << "." << std::setfill('0') << std::setw(3) << mean_ns_fract << "ns" << std::endl
		     << std::endl;
	}
};

#endif /* BENCHMARKABLE_H_ */
