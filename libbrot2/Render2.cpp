/*
    Render2.cpp: Generic rendering functions
    Copyright (C) 2011-2012 Ross Younger

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

#include <png++/png.hpp>
#include <stdlib.h>
#include "Render2.h"
#include "Plot3Chunk.h"
#include "palette.h"
#include "Exception.h"
#include "config.h"

namespace Render2 {

using namespace Plot3;

Base::Base(unsigned width, unsigned height, int local_inf, bool antialias, const BasePalette& pal, bool upscale) :
		_width(width), _height(height), _local_inf(local_inf), _antialias(antialias), _upscale(upscale), _pal(&pal) {
	ASSERT( ! (_antialias && _upscale) ); // These two are not compatible, UI should prevent both being selected
	if (_upscale) {
		ASSERT(!(width%2));
		ASSERT(!(height%2));
	}
}

void Base::process(const Plot3Chunk& chunk)
{
	if (_antialias)
		return process_antialias(chunk);
	else if (_upscale)
		return process_upscale(chunk);
	else
		return process_plain(chunk);
}

void Base::process(const std::list<Plot3Chunk*>& chunks)
{
	std::list<Plot3Chunk*>::const_iterator it;
	for (it = chunks.begin() ; it != chunks.end(); it++) {
		process(**it);
	}
}

void Base::process_plain(const Plot3Chunk& chunk)
{
	const Fractal::PointData * data = chunk.get_data();

	// Slight twist: We've plotted the fractal from a bottom-left origin,
	// but the rest of the universe assumes a top-left origin.

	unsigned i,j;

	// Sanity checks
	ASSERT( chunk._offX + chunk._width <= _width );
	ASSERT( chunk._offY + chunk._height <= _height );

	for (j=0; j<chunk._height; j++) {
		const Fractal::PointData * src = &data[j*chunk._width];

		for (i=0; i<chunk._width; i++) {
			rgb pix = render_pixel(src[i], _local_inf, _pal);
			int xx = i+chunk._offX, yy = _height-(1+j+chunk._offY);
			pixel_done(xx, yy, pix);
		}
	}
}

void Base::process_upscale(const Plot3Chunk& chunk)
{
	const Fractal::PointData * data = chunk.get_data();

	unsigned i,j;
	const unsigned outW = chunk._width * 2,
				   outH = chunk._height * 2,
				   outOffX = chunk._offX * 2,
				   outOffY = chunk._offY * 2;
	ASSERT( chunk._offX + chunk._width <= _width/2 + 1 );
	ASSERT( chunk._offY + chunk._height <= _height/2 + 1);
	ASSERT( outOffX + outW <= _width + 1 );
	ASSERT( outOffY + outH <= _height + 1);

	for (j=0; j<chunk._height; j++) {
		const Fractal::PointData * src = &data[j*chunk._width];

		for (i=0; i<chunk._width; i++) {
			rgb pix = render_pixel(src[i], _local_inf, _pal);
			// Same co-ordinate conversion as in process_plain(), then we upscale
			int xx = 2*(i+chunk._offX), yy = _height - 2 *(1 + j + chunk._offY);
			if ((xx<0) || (yy<0)) continue; // This stops us from running over the edge where output size is not a multiple of 2. We could be fancier here but it's only a draft render so it's not worth the complexity.
			pixel_done(xx+0, yy+0, pix);
			pixel_done(xx+1, yy+0, pix);
			pixel_done(xx+0, yy+1, pix);
			pixel_done(xx+1, yy+1, pix);
		}
	}
}

void Base::process_antialias(const Plot3Chunk& chunk)
{
	const Fractal::PointData * data = chunk.get_data();

	unsigned i,j;
	const unsigned outW = chunk._width / 2,
				   outH = chunk._height / 2,
				   outOffX = chunk._offX / 2,
				   outOffY = chunk._offY / 2;

	ASSERT( chunk._width % 2 == 0);
	ASSERT( chunk._height % 2 == 0);
	ASSERT( chunk._offX + chunk._width <= _width*2 );
	ASSERT( chunk._offY + chunk._height <= _height*2 );
	ASSERT( outOffX + outW <= _width );
	ASSERT( outOffY + outH <= _height);

	for (j=0; j<outH; j++) {
		for (i=0; i<outW; i++) {
			rgb pix[4];

			const Fractal::PointData * base = &data[2*j*chunk._width];
			pix[0] = render_pixel(base[2*i], _local_inf, _pal);
			pix[1] = render_pixel(base[2*i+1], _local_inf, _pal);

			base = &data[(1+2*j)*chunk._width];
			pix[2] = render_pixel(base[2*i], _local_inf, _pal);
			pix[3] = render_pixel(base[2*i+1], _local_inf, _pal);

			pixel_done(i+outOffX, _height-(1+j+outOffY), antialias_pixel4(pix));
		}
	}
}

void Base::fresh_local_inf(unsigned local_inf) {
	_local_inf = local_inf;
}

void Base::fresh_palette(const BasePalette& pal) {
	_pal = &pal;
}

void Base::pixel_overlay(unsigned X, unsigned Y, const rgba& other)
{
	rgb pixel;
	this->pixel_get(X,Y,pixel);
	pixel.overlay(other);
	this->pixel_done(X, Y, pixel);
}

/////////////////////////////////////////////////////////////////////////////////////////////

MemoryBuffer::MemoryBuffer(unsigned char *buf, int rowstride, unsigned width, unsigned height,
		bool antialias, const int local_inf, pixpack_format fmt, const BasePalette& pal, bool upscale) :
					Base(width, height, local_inf, antialias, pal, upscale),
					_buf(buf), _rowstride(rowstride), _fmt(fmt)
{
	ASSERT(buf);
	ASSERT((unsigned)rowstride >= RGB_BYTES_PER_PIXEL * width);

	switch(_fmt) {
	case CAIRO_FORMAT_ARGB32:
	case CAIRO_FORMAT_RGB24:
		_pixelstep = 4;
		break;
	case pixpack_format::PACKED_RGB_24:
		_pixelstep = 3;
		break;
	default:
		THROW(BrotFatalException,"Unhandled pixpack format "+(int)fmt);
		break;
	}
}

MemoryBuffer::~MemoryBuffer()
{
}

void MemoryBuffer::pixel_done(unsigned X, unsigned Y, const rgb& pix)
{
	unsigned char *dst = &_buf[ Y * _rowstride + X * _pixelstep];
	switch(_fmt) {
	case CAIRO_FORMAT_ARGB32:
	case CAIRO_FORMAT_RGB24:
	// Cairo stores its bytes native-endian...
#ifdef WORDS_BIGENDIAN
		dst[0] = 0xff;
		dst[1] = pix.r;
		dst[2] = pix.g;
		dst[3] = pix.b;
#else
		dst[3] = 0xff;
		dst[2] = pix.r;
		dst[1] = pix.g;
		dst[0] = pix.b;
#endif
		dst += _pixelstep; // 4
		break;
		// alpha=1.0 so these cases are the same.
	case pixpack_format::PACKED_RGB_24:
		dst[0] = pix.r;
		dst[1] = pix.g;
		dst[2] = pix.b;
		dst += _pixelstep; // 3
		break;
	}
}

void MemoryBuffer::pixel_get(unsigned X, unsigned Y, rgb& pix)
{
	unsigned char *dst = &_buf[ Y * _rowstride + X * _pixelstep];
	switch(_fmt) {
	case CAIRO_FORMAT_ARGB32:
	case CAIRO_FORMAT_RGB24:
		pix.r = dst[2];
		pix.g = dst[1];
		pix.b = dst[0];
		dst += _pixelstep; // 4
		break;
		// alpha=1.0 so these cases are the same.
	case pixpack_format::PACKED_RGB_24:
		pix.r = dst[0];
		pix.g = dst[1];
		pix.b = dst[2];
		dst += _pixelstep; // 3
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

Writable::Writable(unsigned width, unsigned height, int local_inf, bool antialias, const BasePalette& pal, bool upscale) :
	Base(width, height, local_inf, antialias, pal, upscale) {}
Writable::~Writable() {}

/////////////////////////////////////////////////////////////////////////////////////////////

PNG::PNG(unsigned width, unsigned height,
		const BasePalette& palette, int local_inf, bool antialias, bool upscale) :
				Writable(width, height, local_inf, antialias, palette, upscale),
				_png(_width, _height)
{
}

PNG::~PNG()
{
}

void PNG::pixel_done(unsigned X, unsigned Y, const rgb& pix) {
	_png[Y][X] = png::rgb_pixel(pix.r, pix.g, pix.b);
}

void PNG::pixel_get(unsigned X, unsigned Y, rgb& pix) {
	png::rgb_pixel p = _png[Y][X];
	pix.r = p.red;
	pix.g = p.green;
	pix.b = p.blue;
}


void PNG::write(const std::string& filename)
{
	// TODO: Set png info text (software, comment) once png++ supports this.
	_png.write(filename);
}

void PNG::write(std::ostream& os)
{
	_png.write_stream<std::ostream>(os);
}

/////////////////////////////////////////////////////////////////////////////////////////////

CSV::CSV(unsigned width, unsigned height,
		const BasePalette& palette, int local_inf, bool antialias) :
				Writable(width, height, local_inf, antialias, palette, false) {
	_points = new Fractal::PointData[width*height];
	ASSERT(!_upscale); // Not compatible, this mode only does raw
}

CSV::~CSV() {
	delete[] _points;
}

void CSV::process(const Plot3::Plot3Chunk& chunk) {
	if (_antialias)
		return raw_process_antialias(chunk);
	else
		return raw_process_plain(chunk);
}

void CSV::raw_process_plain(const Plot3::Plot3Chunk& chunk) {
	// This is the same as the original Base::process_plain but with the serial numbers filed off.
	const Fractal::PointData * data = chunk.get_data();
	unsigned i,j;
	// Sanity checks
	ASSERT( chunk._offX + chunk._width <= _width );
	ASSERT( chunk._offY + chunk._height <= _height );
	for (j=0; j<chunk._height; j++) {
		const Fractal::PointData * src = &data[j*chunk._width];
		for (i=0; i<chunk._width; i++) {
			_point(i+chunk._offX, _height-(1+j+chunk._offY)) = src[i];
		}
	}
}

void CSV::raw_process_antialias(const Plot3::Plot3Chunk& chunk) {
	// It doesn't make much sense to average over four fractal points, so we'll just take the base point.
	const Fractal::PointData * data = chunk.get_data();
	unsigned i,j;
	const unsigned outW = chunk._width / 2,
				   outH = chunk._height / 2,
				   outOffX = chunk._offX / 2,
				   outOffY = chunk._offY / 2;
	ASSERT( chunk._width % 2 == 0);
	ASSERT( chunk._height % 2 == 0);
	ASSERT( chunk._offX + chunk._width <= _width*2 );
	ASSERT( chunk._offY + chunk._height <= _height*2 );
	ASSERT( outOffX + outW <= _width );
	ASSERT( outOffY + outH <= _height);
	for (j=0; j<outH; j++) {
		const Fractal::PointData * base = &data[2*j*chunk._width];
		for (i=0; i<outW; i++) {
			_point(i+outOffX, _height-(1+j+outOffY)) = base[2*i];
		}
	}
}

void CSV::write(const std::string& filename) {
	std::ofstream fs;
	fs.open(filename, std::fstream::out);
	write(fs);
	fs.close();
}

std::ostream& operator<<(std::ostream &stream, const Fractal::PointData& pd) {
	stream << "\"" << pd.iterf << "\"";
	return stream;
}

void CSV::write(std::ostream& os) {
	for (unsigned j=0; j<_height; j++) {
		os << _point(0,j);
		for (unsigned i=1; i<_width; i++) {
			os << "," << _point(i,j);
		}
		os << std::endl;
	}

}


// The required abstract functions for dealing with the HUD make no sense here, so we'll just quietly ignore them:
void CSV::pixel_done(unsigned, unsigned, const rgb&) {
}
void CSV::pixel_get(unsigned, unsigned, rgb&) {
}

}; // namespace Render2
