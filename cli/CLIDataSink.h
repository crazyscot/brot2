/*
    CLIDataSink.h: definitions for CLIDataSink.cpp
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
#ifndef REPORTER_H_
#define CLIDATASINK_H

#include <list>
#include "Plot3Plot.h"
#include "IPlot3DataSink.h"

class CLIDataSink : public Plot3::IPlot3DataSink {
	public:
		// Constructor may take an explicit terminal width argument.
		// Otherwise assumes something sensible.
		CLIDataSink(int columns=0, bool silent=false);

		void set_plot(Plot3::Plot3Plot* plot) { _plot = plot; }

		virtual void chunk_done(Plot3::Plot3Chunk* job);
		virtual void pass_complete(std::string&);
		virtual void plot_complete();

		virtual ~CLIDataSink() {}

		std::set<Plot3::Plot3Chunk*> _chunks_done;

	protected:
		int ncolumns;
		bool quiet;
		Plot3::Plot3Plot* _plot;
		std::atomic<int> _chunks_this_pass; // Reset to 0 on pass completion.
};

#endif /* CLIDATASINK_H */
