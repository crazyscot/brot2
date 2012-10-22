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
		Fractal::Point origin(centre - size / 2.0);
		Plot3Chunk * chunk = new Plot3Chunk(s, f, width, height, origin, size, max_passes);
		list_o.push_back(chunk);
	}

	_CD__BODY(Horizontal10px) {
		unsigned nWhole = height / 10;
		unsigned lastPx = height % 10;
		unsigned i;
		const Fractal::Point sliceSize(real(size), imag(size) * 10.0 / height);
		const Fractal::Point step(0.0, imag(sliceSize));
		const Fractal::Point originalOrigin(centre - size / 2.0);

		Fractal::Point origin(originalOrigin);

		for (i=0; i<nWhole; i++) {
			// create and push
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					width, 10,
					origin, sliceSize,
					max_passes);
			list_o.push_back(chunk);
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
		const Fractal::Point lastSize(real(size),
				imag(size) + imag(originalOrigin) - imag(origin));
		{
			Plot3Chunk * chunk = new Plot3Chunk(s, f,
					width, lastPx,
					origin, lastSize,
					max_passes);
			list_o.push_back(chunk);
		}
	}
	// TODO: Vertical slices, horizontal, awkward asymmetric, square superpixels...

};
