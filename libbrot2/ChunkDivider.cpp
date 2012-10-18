/*
    ChunkDivider.cpp: Factory pattern for Plot3 dividing the plot up into chunks
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

#define _CHUNKDIVIDER_INTERNAL
#include "ChunkDivider.h"

#include <memory>
#include "Plot3Chunk.h"
#include "Fractal.h"

namespace ChunkDivider {
	_CD__BODY(OneChunk) {
		Plot3Chunk * chunk = new Plot3Chunk(s, f, centre, size, width, height, max_passes);
		list_o.push_back(chunk);
	}

};
