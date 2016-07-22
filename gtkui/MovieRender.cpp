/*
    MovieRender.cpp: brot2 movie rendering
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

#include "MovieRender.h"
#include "MovieMode.h"

SimpleRegistry<Movie::Renderer> Movie::Renderer::all_renderers;

Movie::Renderer::Renderer(const std::string& _name) : name(_name) {
	all_renderers.reg(_name, this);
}
Movie::Renderer::~Renderer() { }


class ScriptB2CLI : public Movie::Renderer {
	public:
		ScriptB2CLI() : Movie::Renderer("Script for brot2cli") { }

		virtual void render(struct Movie::MovieInfo& movie) {
			// XXX
			(void) movie;
		}
		virtual ~ScriptB2CLI() {}
};

// And now some instances to make them live
static ScriptB2CLI scripter;
