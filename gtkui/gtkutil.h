/*
    gtkutil.h: General GTK miscellanea that didn't fit anywhere else
    Copyright (C) 2011-6 Ross Younger

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

#ifndef GTKUTIL_H_
#define GTKUTIL_H_

#include <gdkmm/color.h>
#include <gtkmm/window.h>
#include <gtkmm/entry.h>
#include <gtkmm/messagedialog.h>
#include <string>

namespace Util {

inline void alert(Gtk::Window *parent, const std::string& message, Gtk::MessageType type = Gtk::MessageType::MESSAGE_ERROR)
{
	Gtk::MessageDialog dialog(*parent, message, false,
			type,
			Gtk::ButtonsType::BUTTONS_OK,
			true);
	dialog.run();
}

/* A subclass of Gtk::Entry with convenient template-driven
 * numeric reading and writing. */
template<typename T>
class HandyEntry: public Gtk::Entry {
	public:
		HandyEntry(unsigned maxsize=0) : Gtk::Entry() {
			if (maxsize) {
				set_max_length(maxsize);
				set_width_chars(maxsize);
			}
		}

		bool read(T& val_out) const {
			Glib::ustring raw = this->get_text();
			std::istringstream tmp(raw, std::istringstream::in);
			// (LP#783087: Don't apply a precision limit to reading digits.)
			T rv=0;
			tmp >> rv;
			if (tmp.fail())
				return false;
			if (!tmp.eof()) // i.e. there was extra stuff we couldn't parse
				return false;
			val_out = rv;
			return true;
		}
		// Write with default precision
		void update(const T val) {
			std::ostringstream tmp;
			tmp << val;
			this->set_text(tmp.str().c_str());
		}
		// Write with explicit precision
		void update(const T val, const int precision) {
			std::ostringstream tmp;
			tmp.precision(precision);
			tmp << val;
			this->set_text(tmp.str().c_str());
		}
// 2nd-order macro for all Gtk::StateType
#define ALL_STATES(DO) \
		DO(NORMAL); \
		DO(ACTIVE); \
		DO(PRELIGHT); \
		DO(SELECTED); \
		DO(INSENSITIVE);
		// Highlights this field, for use when its contents are in error in some way (e.g. unparseable numeric)
		void set_error() {
			Gdk::Color red;
			red.set_rgb(65535, 16384, 16384);
#define HandyEntry_DO_RED(_state) do { modify_base(Gtk::StateType::STATE_##_state, red); } while(0)
			ALL_STATES(HandyEntry_DO_RED)
		}
#define HandyEntry_DO_CLEAR(_state) do { unset_base(Gtk::StateType::STATE_##_state); } while(0)
		void clear_error() {
			ALL_STATES(HandyEntry_DO_CLEAR)
		}
};

// Reads out the geometry for the screen on which the given window appears
void get_screen_geometry(const Gtk::Window& window, int& x, int& y);

// Ensures that the given X and Y co-ordinates lie within a window's screen area, moving them if not
void fix_window_coords(const Gtk::Window& window, int& x, int& y);

}; // namespace Util

#endif // GTKUTIL_H_
