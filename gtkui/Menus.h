/*
    Menus: menu bar for brot2
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

#ifndef MENUS_H_
#define MENUS_H_

#include <gtkmm/menubar.h>

class MainWindow;

namespace menus {

class Menus: public Gtk::MenuBar {
public:
	Menus(MainWindow& parent);

	Gtk::MenuItem main;
	Gtk::MenuItem plot;
	Gtk::MenuItem options;
	Gtk::MenuItem fractal;
	Gtk::MenuItem colour;
};

}

#endif /* MENUS_H_ */
