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

#include "ChunkDivider.h"

#define _CD__BODY(_NAME)		\
	void _NAME::dividePlot(std::list<Plot3Chunk*>& list_o,			\
			IPlot3DataSink* s, const Fractal::FractalImpl& f,		\
			Fractal::Point centre, Fractal::Point size,				\
			unsigned width, unsigned height, unsigned max_passes,	\
			value_e ty) const
	/*
	 * e.g.
	 * _CD__BODY(foo) {
	 * 		... do stuff ...
	 * }
	 */

#include <memory>
#include "Plot3Chunk.h"
#include "Fractal.h"
#include "Exception.h"

using namespace Fractal;

namespace Plot3 {
namespace ChunkDivider {
	_CD__BODY(OneChunk) {
		Fractal::Point origin(centre - size / 2.0);
		Plot3Chunk * chunk = new Plot3Chunk(s, f, width, height, 0, 0, origin, size, ty, max_passes);
		list_o.push_back(chunk);
	}

	_CD__BODY(Horizontal10px) {
		unsigned nWhole = height / 10;
		unsigned lastPx = height % 10;
		const Fractal::Point sliceSize(real(size), imag(size) * 10.0 / height);
		const Fractal::Point step(0.0, imag(sliceSize));
		const Fractal::Point originalOrigin(centre - size / 2.0);

		Fractal::Point origin(originalOrigin);

		/* Note: Co-ordinates (0,0) are at the BOTTOM-LEFT of the render buffer.
		 * For best visual effect, should start at the top and work down. */
		for (unsigned i=0; i<nWhole; i++) {
			// create and push
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					width, 10,
					0, 10*i,
					origin, sliceSize,
					ty, max_passes);
			list_o.push_front(chunk);
			origin += step;
		}
		/* Note: A slight imprecision occurs when repeatedly adding small bits.
		 * Therefore we recompute the last strip to take just what's left of
		 * the complex window.
		 *
		 * We might alternatively compute based on the number of pixels as
		 * a fraction of the whole plot, viz.
		 *     const Fractal::Point lastSize(real(size),
		 *                              imag(size) - nWhole * imag(sliceSize));
		 * but that causes our tests to fail as the bottom-most pixels aren't
		 * sufficiently close in fractal-coordinates to where we computed
		 * them to be (centre + size/2).
		 */
		if (lastPx > 0)	{
			const Fractal::Point lastSize(real(size),
					imag(size) + imag(originalOrigin) - imag(origin));
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					width, lastPx,
					0, 10*nWhole,
					origin, lastSize,
					ty, max_passes);
			list_o.push_back(chunk);
		}
	}

#if 0
	/* Sadly, this is illegal at the moment. Anti-alias mode requires chunk heights
	 * to always be even numbers of pixels. */
	_CD__BODY(Horizontal1px) {
		unsigned nWhole = height-1;
		unsigned lastPx = 1;
		const Fractal::Point sliceSize(real(size), imag(size) * 1.0 / height);
		const Fractal::Point step(0.0, imag(sliceSize));
		const Fractal::Point originalOrigin(centre - size / 2.0);

		Fractal::Point origin(originalOrigin);

		/* Note: Co-ordinates (0,0) are at the BOTTOM-LEFT of the render buffer.
		 * For best visual effect, should start at the top and work down. */
		for (unsigned i=0; i<nWhole; i++) {
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					width, 1,
					0, i,
					origin, sliceSize,
					max_passes);
			list_o.push_front(chunk);
			origin += step;
		}
		// Same inaccuracy as previous.
		if (lastPx > 0)	{
			const Fractal::Point lastSize(real(size),
					imag(size) + imag(originalOrigin) - imag(origin));
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					width, lastPx,
					0, nWhole,
					origin, lastSize,
					max_passes);
			list_o.push_back(chunk);
		}
	}
#endif


	_CD__BODY(Horizontal2px) {
		unsigned nWhole = (height-1) / 2;
		unsigned lastPx = height - 2*nWhole;
		const Fractal::Point sliceSize(real(size), imag(size) * 2.0 / height);
		const Fractal::Point step(0.0, imag(sliceSize));
		const Fractal::Point originalOrigin(centre - size / 2.0);

		Fractal::Point origin(originalOrigin);

		for (unsigned i=0; i<nWhole; i++) {
			// create and push
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					width, 2,
					0, 2*i,
					origin, sliceSize,
					ty, max_passes);
			list_o.push_front(chunk);
			origin += step;
		}
		// Same inaccuracy as previous.
		if (lastPx > 0)	{
			const Fractal::Point lastSize(real(size),
					imag(size) + imag(originalOrigin) - imag(origin));
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					width, lastPx,
					0, 2*nWhole,
					origin, lastSize,
					ty, max_passes);
			list_o.push_back(chunk);
		}
	}

	_CD__BODY(Vertical10px) {
		unsigned nWhole = (width-1) / 10; // Force the last-strip mechanism to always run
		unsigned lastPx = width - 10*nWhole;
		const Fractal::Point sliceSize(real(size) * 10.0 / width, imag(size));
		const Fractal::Point step(real(sliceSize), 0.0);
		const Fractal::Point originalOrigin(centre - size / 2.0);

		Fractal::Point origin(originalOrigin);

		for (unsigned i=0; i<nWhole; i++) {
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					10, height,
					10*i, 0,
					origin, sliceSize,
					ty, max_passes);
			list_o.push_back(chunk);
			origin += step;
		}
		ASSERT(lastPx > 0);
		{
			const Fractal::Point lastSize(
					real(size) + real(originalOrigin) - real(origin),
					imag(size));
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					lastPx, height,
					10*nWhole, 0,
					origin, lastSize,
					ty, max_passes);
			list_o.push_back(chunk);
		}
	}

	void Superpixel::dividePlot(std::list<Plot3Chunk*>& list_o,
			IPlot3DataSink* s, const Fractal::FractalImpl& f,
			Fractal::Point centre, Fractal::Point size,
			unsigned width, unsigned height,
			unsigned max_passes, value_e ty) const {

		unsigned nX = (width-1) / SIZE, nY = (height-1) / SIZE;
		unsigned lastXsize = width - SIZE*nX, lastYsize = height - SIZE*nY;
		const Fractal::Point pixSize(real(size) * (Value)SIZE / width, imag(size) * (Value)SIZE / height);
		const Fractal::Point lastColSize(real(size) * lastXsize / width, imag(size) * (Value)SIZE / height);
		const Fractal::Point lastRowSize(real(size) * (Value)SIZE / width, imag(size) * lastYsize / height);
		const Fractal::Point lastCornerSize(real(size) * lastXsize / width, imag(size) * lastYsize / height);


		const Fractal::Point stepX(real(pixSize), 0.0);
		const Fractal::Point stepY(0.0, imag(pixSize));
		const Fractal::Point originalOrigin(centre - size / 2.0);

		Fractal::Point origin(originalOrigin);

		for (unsigned i=0; i<nY; i++) {
			Fractal::Point thisRowOrigin(origin);
			for (unsigned j=0; j<nX; j++) {
				Plot3Chunk * chunk = new Plot3Chunk(
						s, f, SIZE, SIZE, SIZE*j, SIZE*i,
						thisRowOrigin, pixSize, ty, max_passes);
				list_o.push_front(chunk);
				thisRowOrigin += stepX;
			}
			Plot3Chunk * chunk = new Plot3Chunk(
					s, f, lastXsize, SIZE, (width - lastXsize), SIZE*i,
					thisRowOrigin, lastColSize, ty, max_passes);
			list_o.push_front(chunk);
			origin += stepY;
		}
		// Last row ...
		{
			Fractal::Point thisRowOrigin(origin);
			for (unsigned j=0; j<nX; j++) {
				Plot3Chunk * chunk = new Plot3Chunk(
						s, f, SIZE, lastYsize, SIZE*j, (height-lastYsize),
						thisRowOrigin, lastRowSize, ty, max_passes);
				list_o.push_front(chunk);
				thisRowOrigin += stepX;
			}
			Plot3Chunk * chunk = new Plot3Chunk(
					s, f, lastXsize, lastYsize, (width - lastXsize),
					(height-lastYsize), thisRowOrigin, lastCornerSize, ty, max_passes);
			list_o.push_front(chunk);
			origin += stepY;
		}
	}

} // Plot3::ChunkDivider
} // Plot3
