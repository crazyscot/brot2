/*
    Plot3Pass.cpp: One computation pass of a Plot.
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

#include "Plot3Pass.h"

using namespace std;

namespace Plot3 {

Plot3Pass::Plot3Pass(ThreadPool& pool, std::list<Plot3Chunk*>& chunks) :
	_pool(pool), _chunks(chunks) {
}

Plot3Pass::~Plot3Pass() {
}

void Plot3Pass::run() {
	list<future<void> > results;
	for (auto it=_chunks.begin(); it != _chunks.end(); it++) {
		results.push_back(_pool.enqueue<void>([=]{(*it)->run();}));
	}

	for (auto it=results.begin(); it != results.end(); it++) {
		(*it).get();
	}
}
// TODO Pull in pass-tracking functionality from P3Plot

} // namespace
