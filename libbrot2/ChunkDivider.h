/*
    ChunkDivider.h: Factory pattern for Plot3 dividing the plot up into chunks
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

#ifndef CHUNKDIVIDER_H_
#define CHUNKDIVIDER_H_

#include <list>
#include "Plot3Chunk.h"
#include "Fractal.h"

namespace Plot3 {

namespace ChunkDivider {

	class Base {
		public:
		/*
		 * input: sink, fract, centre, size, width, height, passes_max
		 * output: a load of Plot3Chunk. They have been allocated;
		 * caller must delete each individually when finished with them.
		 */
		virtual void dividePlot(std::list<Plot3Chunk*>& list_o,
				IPlot3DataSink* s, const Fractal::FractalImpl& f,
				Fractal::Point centre, Fractal::Point size,
				unsigned width, unsigned height, unsigned max_passes) const = 0;

		virtual ~Base() {}
	};

#define _CD_INSTANCE(_NAME)  	\
	class _NAME: public Base {	\
		public:					\
		virtual void dividePlot(std::list<Plot3Chunk*>& list_o,			\
				IPlot3DataSink* s, const Fractal::FractalImpl& f,		\
				Fractal::Point centre, Fractal::Point size,				\
				unsigned width, unsigned height, unsigned max_passes) const;	\
	}

	/* NOTE: Chunks must always be an even number of pixels high! */
	_CD_INSTANCE(OneChunk); // Entire fractal as a single chunk
	_CD_INSTANCE(Horizontal10px); // Horizontal stripes, 10pixels high
	_CD_INSTANCE(Horizontal2px); // Ditto, 2px high
	_CD_INSTANCE(Vertical10px); // Vertical stripes, 10pixels wide
	_CD_INSTANCE(Superpixel8x8); // 8x8 superpixels

#undef _CD_INSTANCE
} // Plot3::ChunkDivider

} // Plot3

#endif // CHUNKDIVIDER_H_
