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
#include <sstream>

struct Exception {
	const std::string msg;
	const std::string file;
	const int line;

	Exception(const std::string& m) : msg(m), file(""), line(-1) { }
	Exception(const std::string& m, const std::string& f, int l) : msg(m), file(f), line(l) { }
	virtual std::string detail() const {
		std::stringstream str;
		str << msg;
		if (file.length())
			str << " at " << file << " line " << line;
		else
			str << " (unknown location)";
		return str.str();
	}
	virtual operator const std::string() const {
		return detail();
	}
};

struct Assert : Exception {
	Assert(const std::string& m) :
		Exception("Assertion failed: "+m) { }
	Assert(const std::string& m, const std::string& f, int l) :
		Exception("Assertion failed: "+m,f,l) { }
};

inline std::ostream& operator<< (std::ostream& out, Exception val) {
	out << val.detail();
	return out;
}

#define THROW(type,msg) do { throw type(msg,__FILE__,__LINE__); } while(0)
#define ASSERT(_expr) do { if (!(_expr)) THROW(Assert,#_expr); } while(0)

#endif /* EXCEPTION_H_ */
