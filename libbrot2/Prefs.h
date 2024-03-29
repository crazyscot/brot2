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
#include "Exception.h"
#include "misc.h"
#include <memory> //for unique_ptr
#include <mutex>
BROT2_GLIBMM_BEFORE
#include <glibmm/keyfile.h>
BROT2_GLIBMM_AFTER
#include "PrefsRegistry.h"

struct PrefsException : public BrotException {
	PrefsException(const std::string& m) : BrotException(m) {}
	PrefsException(const std::string& m, const std::string& f, int l) :
		BrotException(m,f,l) {}
	virtual ~PrefsException() {}
};


namespace BrotPrefs {

// Second-order macro to easily define our constants and have their strings to hand.
// ACTION macro takes three args: symbolic constant, numeric constant, friendly string.
#define ALL_ACTIONS(ACTION) \
	ACTION(NO_ACTION, 0, Nothing) \
	ACTION(RECENTRE, 1, Recentre) \
	ACTION(ZOOM_IN, 2, Zoom in) \
	ACTION(ZOOM_OUT, 3, Zoom out) \
	ACTION(DRAG_TO_ZOOM, 4, Drag-to-zoom)

#define FIRST_ACTION NO_ACTION
#define LAST_ACTION DRAG_TO_ZOOM

struct Action {
#define CONSTDEF(_name,_num,_x) static const int _name = _num;
	ALL_ACTIONS(CONSTDEF)

	Action() : value(0) { }

	Action(int val) : value(val) {
		ASSERT((val >= MIN) && (val <= MAX));
	}

	static const int MIN = FIRST_ACTION;
	static const int MAX = LAST_ACTION;

	static std::string name(int n) {
		switch(n) {
#define NAMEIT(_name,_num,_x) case _num: return #_name;
			ALL_ACTIONS(NAMEIT)
		}
		return "???";
	}

	static int lookup(const std::string& n) {
		// Linear search - a bit horrible, but OK for small lists.
#define LOOKUP(_name,_num,_x) if (n.compare(#_name)==0) return _num;
		ALL_ACTIONS(LOOKUP)
		return -1;
	}

	inline std::string name() const {
		ASSERT((value >= MIN) && (value <= MAX));
		return name(value);
	}

	inline Action& operator=(int newval) {
		if ((newval < MIN) || (newval > MAX) )
			THROW(PrefsException,"Illegal enum value");
		value = newval;
		return *this;
	}
	inline operator int() const { return value; }
	inline operator std::string() const { return name(); }

	inline Action& operator=(std::string newname) {
		int newval = lookup(newname);
		if (newval==-1) THROW(PrefsException,"Unrecognised enum string");
		value = newval;
		return *this;
	}

	private:
		int value;
};

template<int _MIN,int _MAX> struct ActionsList {
	static const int MIN = _MIN;
	static const int MAX = _MAX;

	Action a[MAX+1];
	inline Action& operator[] (int i) { ASSERT(i>=_MIN); ASSERT(i<=_MAX); return a[i]; }
	inline Action const& operator[] (int i) const { ASSERT(i>=_MIN); ASSERT(i<=_MAX); return a[i]; }

	void set_to_default(); // Must be defined by each template user!
	static inline ActionsList<_MIN,_MAX> get_default() {
		ActionsList<_MIN,_MAX> rv;
		rv.set_to_default();
		return rv;
	}
	ActionsList<_MIN,_MAX>() { set_to_default(); }
};

// Mouse actions: At least button 8 is used by the Kensington Expert Mouse
typedef ActionsList<1,8> MouseActions;

// Scroll actions:  GDK_SCROLL_{UP,DOWN,LEFT,RIGHT}
typedef ActionsList<0,3> ScrollActions;

///////////////////////////////////////////////////////////
// Putting it all together ...

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
		Prefs(); // not directly instantiable.

	public:
		// Source of a read-only instance of the main live Prefs object.
		// Start with one of these, then (only if you need to) request a
		// working copy for editing.
		//
		// If something went wrong (e.g. backing store I/O error), throws a
		// PrefsException explaining what; it's up to the caller to inform the user.
		static std::shared_ptr<const Prefs> getMaster();

		// Creates a working copy of a Prefs object.
		// Call commit() causes it to update the object it was cloned
		// from with its new contents.
		//
		// The unique_ptr mechanism means you don't have to worry about
		// deleting this object when finished.
		//
		// WARNING: Don't simultaneously create two working copies of
		// a Prefs and expect to be able to commit both of them.
		// Commit (currently) overwrites the entire destination!
		// Because of this it is an error (assert fail) to have more
		// than one working copy outstanding.
		virtual std::shared_ptr<Prefs> getWorkingCopy() const = 0;

		// Commits all outstanding writes of a working copy to the master
		// instance, and thence to backing store.
		// If something went wrong, throws a PrefsException explaining what; it's
		// up to the caller to inform the user suitably.
		virtual void commit() = 0;

		// Data accessors. Note that the getters may change internal state
		// if the relevant backing store did not contain the relevant
		// information, causing a default to be loaded.
		virtual const MouseActions& mouseActions() const = 0;
		virtual void mouseActions(const MouseActions& mouse) = 0;

		virtual const ScrollActions& scrollActions() const = 0;
		virtual void scrollActions(const ScrollActions& scroll) = 0;

		// LP#783034:
		virtual int get(const BrotPrefs::Numeric<int>& B) const = 0;
		virtual void set(const BrotPrefs::Numeric<int>& B, int newval) = 0;
		virtual double get(const BrotPrefs::Numeric<double>& B) const = 0;
		virtual void set(const BrotPrefs::Numeric<double>& B, double newval) = 0;
		virtual bool get(const BrotPrefs::Boolean& B) const = 0;
		virtual void set(const BrotPrefs::Boolean& B, const bool newval) = 0;

		virtual std::string get(const BrotPrefs::String& B) const = 0;
		virtual void set(const BrotPrefs::String& B, const std::string& newval) = 0;

		virtual ~Prefs();
};

class KeyfilePrefs : public Prefs {

public:
	KeyfilePrefs();
	virtual void commit();

	virtual std::shared_ptr<Prefs> getWorkingCopy() const;

	virtual const MouseActions& mouseActions() const;
	virtual void mouseActions(const MouseActions& mouse);

	virtual const ScrollActions& scrollActions() const;
	virtual void scrollActions(const ScrollActions& scroll);

	// LP#783034:
	virtual int get(const BrotPrefs::Numeric<int>& B) const;
	virtual void set(const BrotPrefs::Numeric<int>& B, int newval);
	virtual double get(const BrotPrefs::Numeric<double>& B) const;
	virtual void set(const BrotPrefs::Numeric<double>& B, double newval);
	virtual bool get(const BrotPrefs::Boolean& B) const;
	virtual void set(const BrotPrefs::Boolean& B, const bool newval);
	virtual std::string get(const BrotPrefs::String& B) const;
	virtual void set(const BrotPrefs::String& B, const std::string& newval);

	virtual std::string filename(bool temp=false);
	virtual ~KeyfilePrefs();

protected:
	// Don't forget to add any new fields to the copy constructor if appropriate!
	Glib::KeyFile kf;
	MouseActions mouse_cache;
	ScrollActions scroll_cache;

	KeyfilePrefs* _parent; // NULL if this is the master instance

	static int _childCount; // number of working copies

	KeyfilePrefs(const KeyfilePrefs& src, KeyfilePrefs* parent);

	void reread();
	void initialise();
	void reread_scroll_actions();
	void reread_mouse_actions();

	template<typename T> void ensure(const BrotPrefs::Base<T>& B) {
		try {
			(void)get(B);
		} catch (Glib::KeyFileError& e) {
			set(B, B._default);
		}
	}

	template<typename T> void ensure(const BrotPrefs::Numeric<T>& B) {
		try {
			(void)get(B);
		} catch (Glib::KeyFileError& e) {
			set(B, B._default);
		}
	}

	void ensure(const BrotPrefs::Boolean& B) {
		try {
			(void)get(B);
		} catch (Glib::KeyFileError& e) {
			set(B, B._default);
		}
	}

private:
	// Disallow!
	KeyfilePrefs(const KeyfilePrefs&) = delete;
	KeyfilePrefs& operator= (const KeyfilePrefs&) = delete;
};

class DefaultPrefs : KeyfilePrefs {
public:
	// Source of a read-only instance of the main live Prefs object.
	// Start with one of these, then (only if you need to) request a
	// working copy for editing.
	//
	// If something went wrong (e.g. backing store I/O error), throws a
	// PrefsException explaining what; it's up to the caller to inform the user.
	static std::shared_ptr<const Prefs> getMaster();

private:
	DefaultPrefs(){}; // Not instantiable
	static std::shared_ptr<Prefs> _MASTER;
	static std::mutex _MASTER_lock;
};

// Helper: How big should a threadpool be for a given prefs?
int threadpool_size(std::shared_ptr<const Prefs>);

}; // BrotPrefs

#endif /* PREFS_H_ */
