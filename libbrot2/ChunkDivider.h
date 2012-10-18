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

namespace ChunkDivider {
	class Base {
		public:
		/*
		 * input: sink, fract, centre, size, width, height, passes_max
		 * output: a load of Plot3Chunk. a list? probably take a list&.
		 */
		virtual void dividePlot(std::list<Plot3Chunk*>& list_o,
				IPlot3DataSink* s, const Fractal::FractalImpl* f,
				Fractal::Point centre, Fractal::Point size,
				unsigned width, unsigned height, unsigned max_passes) = 0;

		virtual ~Base() {}
	};

#define _CD_INSTANCE(_NAME)  	\
	class _NAME: public Base {	\
		public:					\
		virtual void dividePlot(std::list<Plot3Chunk*>& list_o,			\
				IPlot3DataSink* s, const Fractal::FractalImpl* f,		\
				Fractal::Point centre, Fractal::Point size,				\
				unsigned width, unsigned height, unsigned max_passes);	\
	}

	_CD_INSTANCE(OneChunk);

#undef _CD_INSTANCE
};

#ifdef _CHUNKDIVIDER_INTERNAL
#define _CD__BODY(_NAME)		\
	void _NAME::dividePlot(std::list<Plot3Chunk*>& list_o,			\
			IPlot3DataSink* s, const Fractal::FractalImpl* f,		\
			Fractal::Point centre, Fractal::Point size,				\
			unsigned width, unsigned height, unsigned max_passes)
	/*
	 * e.g.
	 * _CD__BODY(foo) {
	 * 		... do stuff ...
	 * }
	 */
#endif // _CHUNKDIVIDER_INTERNAL

#endif // CHUNKDIVIDER_H_
