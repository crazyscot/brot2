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
#include "gtkmain.h" // for argv0
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

SimpleRegistry<Movie::Renderer> Movie::Renderer::all_renderers;

Movie::Renderer::Renderer(const std::string& _name) : name(_name) {
	all_renderers.reg(_name, this);
}
Movie::Renderer::~Renderer() {
	all_renderers.dereg(name);
}

void Movie::Renderer::render(const std::string& filename, const struct Movie::MovieInfo& movie) {
	RenderInstancePrivate * priv;
	render_top(filename, movie, &priv);
	// ... go do it XXX
	//
	render_tail(priv);
}

class ScriptB2CLI : public Movie::Renderer {
	class Private : public Movie::RenderInstancePrivate {
		friend class ScriptB2CLI;
		ofstream fs;
		const struct Movie::MovieInfo& movie;
		unsigned fileno;

		Private(const struct Movie::MovieInfo& _movie, const std::string& filename) : movie(_movie), fileno(0) {
			fs.open(filename, std::fstream::out);
		}
		~Private() {
			fs.close();
		}
	};
	public:
		ScriptB2CLI() : Movie::Renderer("Script for brot2cli") { }

		void render_top(const std::string& filename, const struct Movie::MovieInfo& movie, Movie::RenderInstancePrivate **priv) {
			std::string b2cli(brot2_argv0);
			b2cli.append("cli");
			std::string outdir(filename);
			int spos = outdir.rfind('/');
			if (spos >= 0)
				outdir.erase(spos+1);
			Private *mypriv = new Private(movie, filename);
			*priv = mypriv;
			mypriv->fs
				<< "#!/bin/bash" << endl
				<< "# brot2 generated movie script" << endl
				<< endl
				<< "BROT2CLI=\"" << b2cli << "\"" << endl
				<< "OUTDIR=\"" << outdir << "\"" << endl
				<< "OUTNAME=\"render.\"" << endl
				<< endl
				;
		}
		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv) {
			Private * mypriv = (Private*)(priv);
			mypriv->fs
				<< "${BROT2CLI} -X " << fr.centre.real() << " -Y " << fr.centre.imag()
				<< " -l " << fr.size.real()
				<< " -f \"" << mypriv->movie.fractal->name << "\""
				<< " -p \"" << mypriv->movie.palette->name << "\""
				<< " -h " << mypriv->movie.height
				<< " -w " << mypriv->movie.width
				<< " -o \"${OUTDIR}/${OUTNAME}." << setfill('0') << setw(6) << (mypriv->fileno++) << ".png\""
				<< " -q"
				;
			if (mypriv->movie.draw_hud)
				mypriv->fs << " -H";
			if (mypriv->movie.antialias)
				mypriv->fs << " -a";
			mypriv->fs << endl;
		}
		void render_tail(Movie::RenderInstancePrivate *priv) {
			Private * mypriv = (Private*)(priv);
			delete mypriv;
		}
		virtual ~ScriptB2CLI() {}
};

// And now some instances to make them live
static ScriptB2CLI scripter;
