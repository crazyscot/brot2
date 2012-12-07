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
#include <exception>

struct BrotException : std::exception {
	const std::string msg;
	const std::string file;
	const int line;

	BrotException(const std::string& m) : msg(m), file(""), line(-1) { }
	BrotException(const std::string& m, const std::string& f, int l) : msg(m), file(f), line(l) { }
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
	virtual ~BrotException() throw() {}
};

struct BrotAssert : BrotException {
	BrotAssert(const std::string& m) :
		BrotException("Assertion failed: "+m) { }
	BrotAssert(const std::string& m, const std::string& f, int l) :
		BrotException("Assertion failed: "+m,f,l) { }
	virtual ~BrotAssert() throw() {}
};

struct BrotFatalException : BrotException {
	BrotFatalException(const std::string& m) : BrotException("FATAL: "+m) {}
	BrotFatalException(const std::string& m, const std::string& f, int l) :
		BrotException("FATAL: "+m,f,l) {}
	virtual ~BrotFatalException() throw() {}
};

inline std::ostream& operator<< (std::ostream& out, BrotException val) {
	out << val.detail();
	return out;
}

#define THROW(type,msg) do { throw type(msg,__FILE__,__LINE__); } while(0)
#define ASSERT(_expr) do { if (!(_expr)) THROW(BrotAssert,#_expr); } while(0)

#endif /* EXCEPTION_H_ */
