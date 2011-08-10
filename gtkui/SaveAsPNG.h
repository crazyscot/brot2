/*
    SaveAsPNG: PNG export action/support for brot2
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

#ifndef SAVEASPNG_H_
#define SAVEASPNG_H_

class MainWindow;

#include <string>

class SaveAsPNG {
protected:
	static void to_png(MainWindow *mw, std::string& filename);
public:
	static std::string last_saved_dirname;
	static void do_save(MainWindow *mw);
};

#endif /* SAVEASPNG_H_ */