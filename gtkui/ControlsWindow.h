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

#include "Prefs.h"
#include "misc.h"
BROT2_GLIBMM_BEFORE
BROT2_GTKMM_BEFORE
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
BROT2_GTKMM_AFTER
BROT2_GLIBMM_AFTER

class MainWindow;

namespace Actions {
	class MouseButtonsPanel;
	class ScrollButtonsPanel;
};

class ControlsWindow: public Gtk::Window{
	protected:
		MainWindow& mw;
		std::shared_ptr<const BrotPrefs::Prefs> _prefs; // master
		Actions::MouseButtonsPanel* mouse;
		Actions::ScrollButtonsPanel* scroll;

		// Resets the controls to their defaults.
		void on_defaults();

	public:
		ControlsWindow(MainWindow& _mw, std::shared_ptr<const BrotPrefs::Prefs> prefs);
		~ControlsWindow();

		bool close();
		bool on_delete_event(GdkEventAny *evt);
		void starting_position();
};

#endif // CONTROLSWINDOW_H
