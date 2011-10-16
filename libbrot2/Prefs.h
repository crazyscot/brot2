/*
    Prefs.h: Persistent preferences abstraction layer
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

#ifndef PREFS_H_
#define PREFS_H_

#include <string>
#include <assert.h>
#include <Exception.h>

// Second-order macro to easily define our constants and have their strings to hand.
#define ALL_ACTIONS(ACTION) \
	ACTION(NO_ACTION, 0) \
	ACTION(RECENTRE, 1) \
	ACTION(ZOOM_IN, 2) \
	ACTION(ZOOM_OUT, 3) \
	ACTION(DRAG_TO_ZOOM, 4)

#define FIRST_ACTION NO_ACTION
#define LAST_ACTION DRAG_TO_ZOOM

struct Action {
#define CONSTDEF(_name,_num) static const int _name = _num;
	ALL_ACTIONS(CONSTDEF);

	static const int MIN = FIRST_ACTION;
	static const int MAX = LAST_ACTION;

	static std::string name(int n) {
		switch(n) {
#define NAMEIT(_name,_num) case _num: return #_name;
			ALL_ACTIONS(NAMEIT)
		}
		return "???";
	}

	static int lookup(const std::string& n) {
		// Linear search - a bit horrible, but OK for small lists.
#define LOOKUP(_name,_num) if (n.compare(#_name)==0) return _num;
		ALL_ACTIONS(LOOKUP)
		return -1;
	}

	inline std::string name() const {
		assert((value >= MIN) && (value <= MAX));
		return name(value);
	}

	inline void operator=(int newval) throw(Exception) {
		if ((newval < MIN) || (newval > MAX) )
			throw Exception("Illegal enum value");
		value = newval;
	}
	inline operator int() const { return value; }
	inline operator std::string() const { return name(); }

	inline void operator=(std::string newname) throw(Exception) {
		int newval = lookup(newname);
		if (newval==-1) throw Exception("Unrecognised enum string");
		value = newval;
	}

	private:
		int value;
};

struct MouseActions {
	/* What action, if any, does a mouse button event cause? */

	static const int MIN = 1;
	static const int MAX = 9;
	// At least button 8 is used by the Kensington Expert Mouse

	Action a[MAX];
	inline Action& operator[] (unsigned i) { return a[i]; }
	inline Action const& operator[] (unsigned i) const { return a[i]; }

	void set_to_default();
	static inline MouseActions get_default() {
		MouseActions rv;
		rv.set_to_default();
		return rv;
	}
	MouseActions() { set_to_default(); }
};

class Prefs {
	/* This class represents the entire set of preferences that we're
	 * interested in. Implicitly it is connected to a backing store
	 * of some sort, the details of which are not specified in the abstract.
	 * Data elements are not specified as the implementation may (for
	 * example) read/write directly through to backing.
	 *
	 * It might conceivably be useful to have more than one instance of
	 * Prefs at once, so it is not a singleton - though implementations
	 * may be.
	 *
	 * Note that this interface is not limited to returning primitives
	 * and strings. There is no reason why it could not assemble a complicated
	 * struct from the data in backing store, provided it knows how to
	 * default sensibly.
	 */

	protected:
		Prefs();
		virtual ~Prefs();

	public:
		// Most of the time we expect accesses to Prefs will be via this method.
		// If something went wrong (e.g. backing store I/O error), throws an
		// Exception explaining what; it's up to the caller to inform the user.
		static Prefs& getDefaultInstance() throw(Exception);

		// Commits all outstanding writes to backing store. May be a no-op.
		// If something went wrong, throws an Exception explaining what; it's
		// up to the caller to inform the user suitably.
		virtual void commit() throw(Exception) = 0;

		// Data accessors. Note that the getters may change internal state
		// if the relevant backing store did not contain the relevant
		// information, causing a default to be loaded.
		virtual MouseActions mouseActions() = 0;
		virtual void mouseActions(const MouseActions& mouse) = 0;
};


#endif /* PREFS_H_ */
