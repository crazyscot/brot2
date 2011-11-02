/*
    ControlsWindow: brot2 mouse/scroll control window
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

#ifndef CONTROLSWINDOW_H
#define CONTROLSWINDOW_H

#include "Plot2.h"
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>

namespace Actions {
	class MouseButtonsPanel;
	class ScrollButtonsPanel;
};

class ControlsWindow: public Gtk::Dialog {
	protected:
		MainWindow* mw;
		Actions::MouseButtonsPanel* mouse;
		Actions::ScrollButtonsPanel* scroll;

	public:
		ControlsWindow(MainWindow *_mw);
		~ControlsWindow();
		int run(); // Returns the Gtk::ResponseType::RESPONSE_* code selected. If ACCEPT, the MainWindow params have been directly updated.
};


#endif // CONTROLSWINDOW_H
