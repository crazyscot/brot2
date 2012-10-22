/*
    Plot3Chunk.h: A piece of a Plot3.
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

#include "Plot3Chunk.h"
#include "IPlot3DataSink.h"
#include "Exception.h"
#include <complex.h>

using namespace Fractal;

Plot3Chunk::Plot3Chunk(IPlot3DataSink* sink, const Fractal::FractalImpl* f,
		unsigned width, unsigned height, unsigned offX, unsigned offY,
		const Fractal::Point origin, const Fractal::Point size,
		unsigned max_passes) :
		_sink(sink), _data(NULL), _running(false), _prepared(false),
		_live_pixels(0), _max_passes(max_passes),
		_fract(f),
		_origin(origin),
		_size(size),
		_width(width), _height(height), _offX(offX), _offY(offY)
{
	ASSERT(width != 0);
	ASSERT(height != 0);
	ASSERT(real(size) != 0.0);
	ASSERT(imag(size) != 0.0);
}

Plot3Chunk::Plot3Chunk(const Plot3Chunk& other) :
		_sink(other._sink), _data(NULL), _running(false), _prepared(false),
		_live_pixels(0), _max_passes(other._max_passes),
		_fract(other._fract), _origin(other._origin), _size(other._size),
		_width(other._width), _height(other._height), _offX(other._offX), _offY(other._offY)
{
}

Plot3Chunk::~Plot3Chunk() {
	if (_data)
		delete[] _data;
	_data = 0;
}

/* Returns data for a single point, identified by its pixel co-ordinates within the plot. */
const Fractal::PointData& Plot3Chunk::get_pixel_point(int x, int y) const
{
	ASSERT((unsigned)y < _height);
	ASSERT((unsigned)x < _width);
	ASSERT(_data != 0);
	ASSERT(!_running);
	return _data[y * _width + x];
}

void Plot3Chunk::run(job::IJobEngine&) {
	run();
}

void Plot3Chunk::run() {
	ASSERT(!_running);
	_running = true;
	if (!_prepared)
		prepare();
	_prepared = true;
	plot();
	if (_sink)
		_sink->chunk_done(this);
	_running = false;
}

void Plot3Chunk::prepare()
{
	if (_data) delete[] _data;
	_data = new PointData[_width * _height];
	_live_pixels = _width * _height;

	unsigned i,j, out_index = 0;
	//std::cout << "render centre " << centre << "; size " << size << "; origin " << origin << std::endl;

	Point colstep = Point(real(_size) / _width,0);
	Point rowstep = Point(0, imag(_size) / _height);
	//std::cout << "rowstep " << rowstep << "; colstep "<<colstep << std::endl;
	Point render_point = _origin;

	for (j=0; j<_height; j++) {
		for (i=0; i<_width; i++) {
			_fract->prepare_pixel(render_point, _data[out_index]);
			++out_index;
			render_point += colstep;
		}
		render_point.real(real(_origin));
		render_point += rowstep;
	}
}

void Plot3Chunk::plot() {
	unsigned i, j, out_index = 0;
	for (j=0; j<_height; j++) {
		for (i=0; i<_width; i++) {
			PointData& pt = _data[out_index];
			if (!pt.nomore) {
				_fract->plot_pixel(_max_passes, pt);
				if (pt.nomore)
					--_live_pixels;
			}
			++out_index;
		}
	}
}

void Plot3Chunk::reset_max_passes(unsigned max) {
	ASSERT(!_running);
	_max_passes = max;
}
