/*
    MovieMode: brot2 movie plotting
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

#ifndef MOVIEMODE_H
#define MOVIEMODE_H

#include "Prefs.h"
#include "Fractal.h"
#include "FractalMaths.h"
#include "palette.h"
#include "ThreadPool.h"
#include "IMovieProgress.h"
#include <vector>

namespace Movie {

struct KeyFrame {
	Fractal::Point centre, size;
	unsigned hold_frames; // How many video frames to linger on this render
	unsigned speed_zoom, speed_translate; // Speeds for motion to next key frame (ignored if this is last)
	bool ease_in, ease_out;
	// Centre, Axis size, Hold frames, Speed Zoom, Speed Translate
	KeyFrame(Fractal::Point _c, Fractal::Point _s, unsigned _h, unsigned _sz, unsigned _st, bool _ease_in=false, bool _ease_out=false) :
		centre(_c), size(_s), hold_frames(_h), speed_zoom(_sz), speed_translate(_st), ease_in(_ease_in), ease_out(_ease_out) {}
	KeyFrame( Fractal::Value _cr, Fractal::Value _ci,
	// Centre Re/Im, Axis size Re/Im, Hold frames, Speed Zoom, Speed Translate
			Fractal::Value _sr, Fractal::Value _si,
			unsigned _h, unsigned _sz, unsigned _st, bool _ease_in=false, bool _ease_out=false) :
		centre(_cr, _ci), size(_sr, _si), hold_frames(_h), speed_zoom(_sz), speed_translate(_st), ease_in(_ease_in), ease_out(_ease_out) {}

	double zoom() const; // Computes the zoom factor (based on the real axis length)
	double logzoom() const;
};

struct Frame {
	Fractal::Point centre, size;
	Frame() : centre(0,0), size(0,0) {}
	Frame(Fractal::Point _c, Fractal::Point _s) : centre(_c), size(_s) {}
	Frame( Fractal::Value _cr, Fractal::Value _ci,
			Fractal::Value _sr, Fractal::Value _si) : centre (_cr, _ci), size (_sr, _si) {}
	Frame(struct KeyFrame kf) : centre(kf.centre), size(kf.size) {}
};

// Describes the motion from one keyframe to the next
struct Vector {
	Vector(const struct KeyFrame& _f1, const struct KeyFrame& _f2);
	Vector(Fractal::Value _x, Fractal::Value _y, Fractal::Value _z) : x(_x), y(_y), z(_z) {}

	KeyFrame apply(const KeyFrame& _base) const;

	static Fractal::Value real_axis_from_z(Fractal::Value z);

	Fractal::Value x, y, z; /* x real, y imag, z zoom factor */
};

struct MovieInfo {
	const Fractal::FractalImpl * fractal;
	const BasePalette * palette;
	unsigned width, height;
	bool draw_hud, antialias, preview;
	unsigned fps;
	std::vector<KeyFrame> points;

	MovieInfo() : fractal(0), palette(0), width(0), height(0), draw_hud(false), antialias(true), preview(false), fps(0) {}
	unsigned count_frames() const;

	private:
		static std::shared_ptr<ThreadPool> movieinfo_runner_thread;
};

class NullCompletionHandler : public IMovieCompleteHandler {
	public:
		virtual void signal_completion(RenderJob&) {}
		virtual void signal_error(RenderJob&, const std::string&) {}
		virtual ~NullCompletionHandler() {}
};


}; // namespace Movie

#endif // MOVIEMODE_H
