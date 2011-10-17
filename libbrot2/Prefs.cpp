/*
    Prefs.cpp: Persistent preferences abstraction layer
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

#include "Prefs.h"

#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <glibmm/keyfile.h>
#include <glibmm/fileutils.h>

class KeyfilePrefs;

#define META_GROUP "meta"
#define KEY_VERSION "version"
#define CURRENT_VERSION 1

#define GROUP_MOUSE "mouse_actions"
#define GROUP_SCROLL "scroll_actions"

template<>
void MouseActions::set_to_default() {
	a[1] = Action::RECENTRE;
	a[2] = Action::ZOOM_OUT;
	a[3] = Action::DRAG_TO_ZOOM;
	a[4] = Action::NO_ACTION;
	a[5] = Action::NO_ACTION;
	a[6] = Action::NO_ACTION;
	a[7] = Action::NO_ACTION;
	a[8] = Action::ZOOM_IN;
}

template<>
void ScrollActions::set_to_default() {
	a[0] = Action::ZOOM_IN; // GDK_SCROLL_UP
	a[1] = Action::ZOOM_OUT; // GDK_SCROLL_DOWN
	a[2] = Action::NO_ACTION; // GDK_SCROLL_LEFT
	a[3] = Action::NO_ACTION; // GDK_SCROLL_RIGHT
}

class KeyfilePrefs : public Prefs {
	private:
		Glib::KeyFile kf;
		MouseActions mouse_cache;
		ScrollActions scroll_cache;

	protected:
		~KeyfilePrefs() {
			// Do NOT commit here.
		}

		std::string filename(bool temp=false) {
			std::string rv("");
			char *home = getenv("HOME");
			if (home != NULL)
				rv = rv + home + '/';
			rv = rv + ".brot2";
			if (temp)
				rv = rv + ".tmp";
			return rv;
		}

	public:
		KeyfilePrefs() throw(Exception) {
			kf.set_comment("generated by brot2");
			kf.set_integer(META_GROUP, KEY_VERSION, CURRENT_VERSION);

			std::string fn = filename();
			try {
				kf.load_from_file(fn);
			} catch (Glib::FileError e) {
				switch (e.code()) {
					case Glib::FileError::Code::NO_SUCH_ENTITY:
						break; // ignore, use defaults only
					default:
						throw Exception("reading prefs from " + fn + ": " + e.what());
				}
			} catch (Glib::KeyFileError e) {
				switch (e.code()) {
					case Glib::KeyFileError::Code::PARSE:
						// may mean an empty file
						std::cerr << "Warning: KeyFileError reading prefs from " + fn + ": " + e.what()+": will overwrite when saving" << std::endl;
						break;
					default:
						throw Exception("KeyFileError reading prefs from " + fn + ": " + e.what());
				}
			}

			// Pre-populate our caches.
			reread_mouse_actions();
			reread_scroll_actions();
		}

		virtual void commit() throw(Exception) {
			int rv;
			std::string fn = filename(true); // write to foo.tmp
			std::ofstream f;

			rv = unlink(fn.c_str());
			if (rv==-1) {
				switch(errno) {
					case ENOENT: 
						break; //ignore
					default:
						throw Exception("Could not unlink " + fn + ": " + strerror(errno));
				}
			}
			kf.set_comment("written by brot2");

			std::ostringstream acts;
			acts << Action::name(Action::MIN);
			for (int i=Action::MIN+1; i<=Action::MAX; i++)
				acts << ", " << Action::name(i);

			if (!kf.has_group(GROUP_MOUSE))
				mouseActions(mouse_cache);
			kf.set_comment(GROUP_MOUSE, "Mouse button actions. Supported actions are: " + acts.str());

			acts.str("");
			acts.clear();
			bool seenone = false;
			for (int i=Action::MIN; i<=Action::MAX; i++) {
				// We don't know how to handle drag-to-zoom on a scroll event.
				if (i != Action::DRAG_TO_ZOOM) {
					if (seenone)
						acts << ", ";
					acts << Action::name(i);
					seenone = true;
				}
			}
			if (!kf.has_group(GROUP_SCROLL))
				scrollActions(ScrollActions());
			kf.set_comment(GROUP_SCROLL, "Scroll wheel actions. Supported actions are: " + acts.str());
			// (However we won't enforce its absence, we'll just ignore it if it turns up.)

			// !!! Adding new options or groups? Ensure they are set to reasonable defaults here.

			f.open(fn);
			f << kf.to_data();
			f.close();

			// And rename new on top of old.
			std::string newfn = filename();
			rv = rename(fn.c_str(), newfn.c_str());
			if (rv==-1) {
				throw Exception("Could not rename " + fn + " to " + newfn + ": " + strerror(errno));
			}
		}

		/* MOUSE ACTIONS:
		 * Store as a group of a suitable name.
		 * Keys are named act_N with values from the Action enum. */
		virtual void mouseActions(const MouseActions& mouse) {
			mouse_cache=mouse;
			for (int i=mouse.MIN; i<=mouse.MAX; i++) {
				char buf[32];
				snprintf(buf, sizeof buf, "action_%d", i);
				kf.set_string(GROUP_MOUSE, buf, mouse[i].name());
			}
		}
		virtual const MouseActions& mouseActions() {
			return mouse_cache;
		}

		void reread_mouse_actions() {
			MouseActions rv; // Constructor sets to defaults
			for (int i=rv.MIN; i<=rv.MAX; i++) {
				char buf[32];
				snprintf(buf, sizeof buf, "action_%d", i);
				try {
					Glib::ustring val = kf.get_string(GROUP_MOUSE, buf);
					rv[i] = val;
				} catch (Glib::KeyFileError e) {
					// ignore - use default for that action
				} catch (Exception e) {
					std::cerr << "Warning: " << e << " in " GROUP_MOUSE <<":" << buf << ": defaulting" << std::endl;
				}
			}
			mouse_cache = rv;
		}

		// FIXME: Figure out a way to template my way out of this clone and hack:
		/* SCROLL ACTIONS:
		 * Store as a group of a suitable name.
		 * Keys are named act_N with values from the Action enum. */
		virtual void scrollActions(const ScrollActions& scroll) {
			scroll_cache = scroll;
			for (int i=scroll.MIN; i<=scroll.MAX; i++) {
				char buf[32];
				snprintf(buf, sizeof buf, "action_%d", i);
				kf.set_string(GROUP_SCROLL, buf, scroll[i].name());
			}
		}
		virtual const ScrollActions& scrollActions() {
			return scroll_cache;
		}
		void reread_scroll_actions() {
			ScrollActions rv; // Constructor sets to defaults
			for (int i=rv.MIN; i<=rv.MAX; i++) {
				char buf[32];
				snprintf(buf, sizeof buf, "action_%d", i);
				try {
					Glib::ustring val = kf.get_string(GROUP_SCROLL, buf);
					rv[i] = val;
				} catch (Glib::KeyFileError e) {
					// ignore - use default for that action
				} catch (Exception e) {
					std::cerr << "Warning: " << e << " in " GROUP_SCROLL <<":" << buf << ": defaulting" << std::endl;
				}
			}
			scroll_cache=rv;
		}

};

namespace {
	KeyfilePrefs *gtkPrefs = 0;
};

// Default accessor, singleton-like.
Prefs& Prefs::getDefaultInstance() throw (Exception) {
	if (gtkPrefs == NULL)
		gtkPrefs = new KeyfilePrefs();
	return *gtkPrefs;
};

Prefs::Prefs() { }
Prefs::~Prefs() { }

