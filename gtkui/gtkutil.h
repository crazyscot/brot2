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
};


}; // namespace Util

#endif // GTKUTIL_H_
