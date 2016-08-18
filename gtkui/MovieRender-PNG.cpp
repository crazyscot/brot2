/*
    MovieRender-PNG.cpp: brot2 movie rendering, PNG output
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
#include "misc.h"
#include "gtkutil.h"
#include "SaveAsPNG.h"
#include <iostream>
#include <iomanip>
#include <errno.h>

using namespace std;

// ------------------------------------------------------------------------------
// Outputs a bunch of PNGs in a single directory

class BunchOfPNGs : public Movie::Renderer {
	class Private : public Movie::RenderInstancePrivate {
		friend class BunchOfPNGs;
		unsigned fileno;
		const std::string outdir, nametmpl;

		Private(Movie::RenderJob& _job, const std::string& _outdir, const std::string& _tmpl) :
			RenderInstancePrivate(_job), fileno(0), outdir(_outdir), nametmpl(_tmpl)
		{
		}
		virtual ~Private() {}
	};
	public:
		BunchOfPNGs() : Movie::Renderer("PNG files in a directory", "*.png") { }

		void render_top(Movie::RenderJob& job, Movie::RenderInstancePrivate **priv) {
			std::string outdir(job._filename), tmpl(job._filename);
			int spos = outdir.rfind('/');
			if (spos >= 0) {
				outdir.erase(spos+1);
				tmpl.erase(0, spos+1);
			}
			spos = tmpl.rfind(".png");
			if (spos >= 0)
				tmpl.erase(spos);
			Private *mypriv = new Private(job, outdir, tmpl);
			*priv = mypriv;
		}
		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv, unsigned n_frames) {
			Private * mypriv = (Private*)(priv);
			ostringstream filename;
			filename << mypriv->outdir << '/' << mypriv->nametmpl << '_' << setfill('0') << setw(6) << (mypriv->fileno++) << ".png";
			std::string filename2 (filename.str());

			SavePNG::MovieFrame saver(mypriv->job._prefs, mypriv->job._threads,
					*mypriv->job._movie.fractal, *mypriv->job._movie.palette,
					*mypriv->job._reporter,
					fr.centre, fr.size,
					mypriv->job._movie.width, mypriv->job._movie.height,
					mypriv->job._movie.antialias, mypriv->job._movie.draw_hud,
					filename2);
			saver.start();
			mypriv->job._reporter->set_chunks_count(saver.get_chunks_count());
			saver.wait();
			saver.save_png(0);
			for (unsigned i=1; i<n_frames; i++) {
				ostringstream file_copy;
				file_copy << mypriv->outdir << '/' << mypriv->nametmpl << '_' << setfill('0') << setw(6) << (mypriv->fileno++) << ".png";
				if (Util::copy_file(filename2, file_copy.str()) != 0) {
					ostringstream msg;
					msg << "Error copying frames: " << strerror(errno);
					Util::alert(0, msg.str());
				}
			}
			// LATER Could kick off all frames in parallel and wait for them in render_bottom?
		}
		void render_tail(Movie::RenderInstancePrivate *priv, bool) {
			Private * mypriv = (Private*)(priv);
			delete mypriv;
		}
		virtual ~BunchOfPNGs() {}
};

// ------------------------------------------------------------------------------
// And now some instances to make them live

MOVIERENDER_DECLARE_FACTORY(BunchOfPNGs, "PNG files in a directory", "*.png");
