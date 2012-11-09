/*
    SaveAsPNG: PNG export action/support for brot2
    Copyright (C) 2011-12 Ross Younger

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

#ifndef SAVEASPNG_H_
#define SAVEASPNG_H_

class MainWindow;

#include "Plot3Plot.h"
#include "palette.h"
#include "ChunkDivider.h"
#include <string>

class PNGProgressWindow;

class SaveAsPNG {
	friend class MainWindow;

	// Private constructor! Called by do_save().
	SaveAsPNG(MainWindow* mw, Plot3::Plot3Plot& oldplot, unsigned width, unsigned height, bool antialias, std::string&name);

	// Interface for MainWindow to trigger save actions.
	// An instance of this class is an outstanding PNG-save job.
private:
	static void to_png(MainWindow *mw, unsigned rwidth, unsigned rheight,
			Plot3::Plot3Plot* plot, BasePalette* pal, bool antialias,
			std::string& filename);

	// Delete on destruct:
	PNGProgressWindow reporter;
	Plot3::ChunkDivider::Horizontal10px divider;
	const int aafactor;
	Plot3::Plot3Plot plot;

	// do NOT delete:
	BasePalette *pal;
	std::string filename;

	void start(); // ->plot.start()
	void wait(); // -> plot.wait()

public:
	static std::string last_saved_dirname;

	// main entrypoint: runs the save dialog and DTRTs
	static void do_save(MainWindow *mw);

	virtual ~SaveAsPNG();
};

#endif /* SAVEASPNG_H_ */
