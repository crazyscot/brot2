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
		// Don't forget to add any new fields to the copy constructor if appropriate!
		Glib::KeyFile kf;
		MouseActions mouse_cache;
		ScrollActions scroll_cache;

		KeyfilePrefs* _parent; // NULL if this is the master instance

		static int _childCount; // number of working copies

	private:
		KeyfilePrefs(const KeyfilePrefs& src, KeyfilePrefs* parent) : _parent(parent) {
			// Because we're cloning from a read-only instance this is
			// actually the same as reading from scratch.
			initialise();
			mouse_cache = src.mouse_cache;
			scroll_cache = src.scroll_cache;
			++_childCount;
		}

	protected:
		~KeyfilePrefs() {
			--_childCount;
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
		KeyfilePrefs() throw(Exception) : _parent(NULL) {
			initialise();
		}
	private:
		void reread() throw (Exception) {
			initialise();
		}

		void initialise() throw(Exception) {
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

			// Pre-populate our caches and defaults for any new prefs.
			reread_mouse_actions();
			reread_scroll_actions();

			ensure(PREF(ShowControls));
			ensure(PREF(InitialMaxIter));
			ensure(PREF(LiveThreshold));
			ensure(PREF(MinEscapeePct));
		}

		virtual void commit() throw(Exception) {
			// This is sneaky... We write to the backing store, and
			// prod the parent to reread.
			if (!_parent)
				throw Assert("commit called on unparented KeyFilePrefs");
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

			_parent->reread();
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
		virtual const MouseActions& mouseActions() const {
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
		virtual const ScrollActions& scrollActions() const {
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

		virtual std::unique_ptr<Prefs> getWorkingCopy() const throw(Exception);

		// LP#783034:
		virtual int get(const BrotPrefs::Numeric<int>& B) const;
		virtual void set(const BrotPrefs::Numeric<int>& B, int newval);
		virtual double get(const BrotPrefs::Numeric<double>& B) const;
		virtual void set(const BrotPrefs::Numeric<double>& B, double newval);
		virtual bool get(const BrotPrefs::Base<bool>& B) const;
		virtual void set(const BrotPrefs::Base<bool>& B, const bool newval);

		template<typename T> void ensure(const BrotPrefs::Base<T>& B) {
			try {
				(void)get(B);
			} catch (Glib::KeyFileError e) {
				set(B, B._default);
			}
		}
		template<typename T> void ensure(const BrotPrefs::Numeric<T>& B) {
			try {
				(void)get(B);
			} catch (Glib::KeyFileError e) {
				set(B, B._default);
			}
		}
};

int KeyfilePrefs::get(const BrotPrefs::Numeric<int>& B) const {
	int rv = kf.get_integer(B._group, B._key);
	if ((rv < B._min) || (rv > B._max))
		rv = B._default;
	return rv;
}
void KeyfilePrefs::set(const BrotPrefs::Numeric<int>& B, int newval) {
	if ((newval < B._min) || (newval > B._max)) {
		throw Exception("Pref set out of range");
	}
	kf.set_integer(B._group, B._key, newval);
}

double KeyfilePrefs::get(const BrotPrefs::Numeric<double>& B) const {
	double rv = kf.get_double(B._group, B._key);
	if ((rv < B._min) || (rv > B._max))
		rv = B._default;
	return rv;
}
void KeyfilePrefs::set(const BrotPrefs::Numeric<double>& B, double newval) {
	if ((newval < B._min) || (newval > B._max)) {
		throw Exception("Pref set out of range");
	}
	kf.set_double(B._group, B._key, newval);
}

bool KeyfilePrefs::get(const BrotPrefs::Base<bool>& B) const {
	return kf.get_boolean(B._group, B._key);
}
void KeyfilePrefs::set(const BrotPrefs::Base<bool>& B, const bool newval) {
	kf.set_boolean(B._group, B._key, newval);
}

namespace {
	KeyfilePrefs *gtkPrefs = 0;
};

// Default accessor, singleton-like.
const Prefs& Prefs::getMaster() throw(Exception) {
	if (gtkPrefs == NULL)
		gtkPrefs = new KeyfilePrefs();
	return *gtkPrefs;
}

std::unique_ptr<Prefs> KeyfilePrefs::getWorkingCopy() const throw(Exception) {
	if (_parent != NULL)
		throw Exception("Prefs: Cannot make a working copy of a working copy!");
	if (_childCount)
		throw Exception("Prefs: cannot make a working copy when there's one outstanding");

	Prefs *rv = new KeyfilePrefs(*this, const_cast<KeyfilePrefs*>(this));
	std::unique_ptr<Prefs> prv(rv);
	return prv;
}

Prefs::Prefs() { }
Prefs::~Prefs() { }
int KeyfilePrefs::_childCount = 0;

