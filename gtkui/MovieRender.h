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

#ifndef MOVIERENDER_H
#define MOVIERENDER_H

#include "MovieMode.h"
#include "Prefs.h"
#include "Fractal.h"
#include "FractalMaths.h"
#include "palette.h"
#include "Registry.h"
#include <vector>

namespace Movie {

class Renderer;
class RenderInstancePrivate {}; // Used by Renderer to store private data

class Renderer {

	protected:
		Renderer(const std::string& name);

	public:
		const std::string name;

		void render(const std::string& filename, const struct Movie::MovieInfo& movie);

		// Initialise render run, alloc Private if needed
		virtual void render_top(const std::string& filename, const struct Movie::MovieInfo& movie, Movie::RenderInstancePrivate** priv) = 0;

		// Called for each keyframe
		virtual void render_each(const struct Movie::KeyFrame& kf, Movie::RenderInstancePrivate *priv) = 0;

		// Finish up, flush file, delete Private
		virtual void render_tail(Movie::RenderInstancePrivate *priv) = 0;

		virtual ~Renderer();

		static SimpleRegistry<Renderer> all_renderers;
};

}; // namespace Movie

#endif // MOVIERENDER_H
