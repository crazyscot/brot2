/*
    misc.h: General miscellanea that didn't fit anywhere else
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

#ifndef MISC_H_
#define MISC_H_

#include <sys/time.h>
#include <string>

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

namespace Util {

inline struct timeval tv_subtract (struct timeval tv1, struct timeval tv2)
{
	struct timeval rv;
	rv.tv_sec = tv1.tv_sec - tv2.tv_sec;
	if (tv1.tv_usec < tv2.tv_usec) {
		rv.tv_usec = tv1.tv_usec + 1e6 - tv2.tv_usec;
		--rv.tv_sec;
	} else {
		rv.tv_usec = tv1.tv_usec - tv2.tv_usec;
	}

	return rv;
}

class xy {
	// Represents a point in some integer space
public:
	int x, y;
	xy() : x(0), y(0) {};
	xy(int &xx, int &yy) : x(xx), y(yy) {};
	void reinit(int xx, int yy) { x=xx; y=yy; }
};

inline bool ends_with(std::string const & value, std::string const & ending)
{
	/* Found on Stack Overflow */
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

/* Copies a file from one filename to another.
 * On error, returns -1 and sets errno. */
int copy_file(const std::string& from, const std::string& to);

}; // namespace Util

#endif // MISC_H_
