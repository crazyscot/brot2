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
		// If something went wrong (e.g. backing store I/O error), throws a
		// String explaining what; it's up to the caller to inform the user.
		static Prefs& getDefaultInstance() throw(std::string);

		// Commits all outstanding writes to backing store. May be a no-op.
		// If something went wrong, throws a String explaining what; it's
		// up to the caller to inform the user suitably.
		virtual void commit() throw(std::string) = 0;

		// Data accessors. Note that the getters may change internal state
		// if the relevant backing store did not contain the relevant
		// information, causing a default to be loaded.
		virtual int foo() = 0;
		virtual void foo(int newfoo) = 0;
		static int default_foo();
};


#endif /* PREFS_H_ */
