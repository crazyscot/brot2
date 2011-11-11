/*
    Exception.h: Exception holding class
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

#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <string>
#include <iostream>

struct Exception {
	const std::string msg;

	Exception(const std::string m) : msg(m) { }
	virtual operator const std::string&() const { return msg; }
};

struct Assert : Exception {
	Assert(const std::string m) : Exception(m) { }
};

inline std::ostream& operator<< (std::ostream& out, Exception val) {
	out << val.msg;
	return out;
}

#endif /* EXCEPTION_H_ */
