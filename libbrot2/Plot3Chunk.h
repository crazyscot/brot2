/*
    Plot3Chunk.h: A piece of a Plot3Plot.
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

#ifndef PLOT3CHUNK_H_
#define PLOT3CHUNK_H_

#include "Fractal.h"

namespace Plot3 {

class IPlot3DataSink;

class Plot3Chunk {
public:
	// If sink is not null, we will pass our result data to it when complete.
	Plot3Chunk(IPlot3DataSink* sink, const Fractal::FractalImpl& f,
			unsigned width, unsigned height, unsigned offX, unsigned offY,
			const Fractal::Point origin, const Fractal::Point size,
			Fractal::Maths::MathsType ty, unsigned max_passes=0);
	Plot3Chunk(const Plot3Chunk& other);
	virtual ~Plot3Chunk();

	virtual void run();
protected:
	virtual void prepare();
	virtual void plot();

private:
	const Plot3Chunk& operator= (const Plot3Chunk&) = delete; // Disallowed.

    /* Where should this chunk poke its data when complete? */
	IPlot3DataSink* _sink;
	Fractal::PointData* _data; // We own this data. Allocated when needed.
	bool _running, _prepared;
	/* Plot statistics: */
	unsigned _plotted_passes; // How many passes before bailing?
	unsigned _live_pixels; // How many pixels are still live? Initialised by prepare().
	unsigned _max_passes; // Do we have an absolute limit on the number of passes?

public:
	/* What is this chunk about? */
	const Fractal::FractalImpl& _fract;
	const Fractal::Point _origin, _size; // Origin co-ordinates; axis length (cannot be 0 in either dimension)
	const unsigned _width, _height; // plot size in pixels, starting from 0
	const unsigned _offX, _offY; // This chunk's pixel offset into the plot
	const Fractal::Maths::MathsType _valtype; // What size of arithmetic are we doing?

	unsigned pixel_count() const { return _width * _height; }
	const Fractal::Point centre() const { return _origin + _size/(Fractal::Value)2.0; }
	unsigned maxiter() const { return _max_passes; }

	/* Read-only access to the plot data. There are pixel_count() pixels. */
	const Fractal::PointData * get_data() const { if (!this) return 0; return _data; }

	// How many pixels are live?
	unsigned livecount() const { return _live_pixels; }

	/* Returns data for a single point, identified by its pixel co-ordinates within.
	 * NB that the pixel co-ords are relative to this chunk only.
	 * Call this before completion at your peril... */
	const Fractal::PointData& get_pixel_point(int x, int y) const;

	/** Updates our idea of the iteration limit */
	void reset_max_passes(unsigned max);
};

} // namespace Plot3

#endif /* PLOT3CHUNK_H_ */
