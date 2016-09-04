/*
    MovieNullProgress.cpp: Dummy movie progress reporter
    Copyright (C) 2016 Ross Younger

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

#include "IMovieProgress.h"

Movie::MovieNullProgress::MovieNullProgress() {}
Movie::MovieNullProgress::~MovieNullProgress() {}

void Movie::MovieNullProgress::set_chunks_count(int) {}
void Movie::MovieNullProgress::frames_traversed(int) {}

void Movie::MovieNullProgress::chunk_done(Plot3::Plot3Chunk*) {}
void Movie::MovieNullProgress::pass_complete(std::string&, unsigned, unsigned, unsigned, unsigned) {}
void Movie::MovieNullProgress::plot_complete() {}

