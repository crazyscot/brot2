/*
    PrefsRegistry.h: Typed helpers for keeping track of the prefs
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

#ifndef PREFSREGISTRY_H_
#define PREFSREGISTRY_H_

#include <string>

namespace BrotPrefs {

template<typename T>
struct Base {
	const T _default;
	const std::string _name, _description, _group, _key;

	Base( 	const std::string name,
			const std::string desc,
			const T& def,
			const std::string grp, const std::string key) :
		_default(def), _name(name), _description(desc),
		_group(grp), _key(key) { }
};

template<typename N>
struct Numeric : public Base<N> {
	const N _min, _max;
	Numeric(const std::string name,
			const std::string desc,
			const N& min,
			const N& def,
			const N& max,
			const std::string grp, const std::string key) :
		Base<N>(name, desc, def, grp, key), _min(min), _max(max) { }
};

typedef Numeric<int> Int;
typedef Numeric<double> Float;

//typedef Base<bool> Bool; // not legal C++, alas

struct Registry {
	Int InitialMaxIter;
	Float LiveThreshold;
	Int MinEscapeePct;

	static const Registry& get();
	private:
		static Registry *_instance;
		Registry();
};

}; // namespace BrotPrefs

// And a set of syntactic sugar macros...
#define PREF(_member) (BrotPrefs::Registry::get()._member)
#define PREFNAME(_member) (PREF(_member)._name)
#define PREFDESC(_member) (PREF(_member)._description)


#endif /* PREFSREGISTRY_H_ */
