/*
    reporter.h: definitions for reporter.cpp
    Copyright (C) 2011 Ross Younger

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
#define REPORTER_H_

#include "Plot2.h"

class Reporter : public Plot2::callback_t {
	public:
		// Constructor may take an explicit terminal width argument.
		// Otherwise assumes something sensible.
		Reporter(int columns=0, bool silent=false);

		virtual void plot_progress_minor(Plot2& plot, float workdone);
		virtual void plot_progress_major(Plot2& plot, unsigned current_maxiter, std::string& commentary);
		virtual void plot_progress_complete(Plot2& plot);
	protected:
		int ncolumns;
		bool quiet;
};

#endif /* REPORTER_H_ */
