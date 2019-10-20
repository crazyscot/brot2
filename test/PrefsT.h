/*  TestPrefs.cpp: brot2 prefs mechanism unit tests
    Copyright (C) 2011-2 Ross Younger

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

#ifndef TESTPREFS_H_
#define TESTPREFS_H_

#include "Prefs.h"

class TestingKeyfilePrefs: public BrotPrefs::KeyfilePrefs {
public:
	// Need to use named constructors as our initialise() calls a virtual function :-(
	static TestingKeyfilePrefs* getInstance();
	static TestingKeyfilePrefs* getInstance(const BrotPrefs::KeyfilePrefs& src, BrotPrefs::KeyfilePrefs* parent);
	virtual ~TestingKeyfilePrefs();

	virtual std::shared_ptr<BrotPrefs::Prefs> getWorkingCopy() const throw();
	virtual std::string filename(bool temp=false);

protected:
	TestingKeyfilePrefs();
	TestingKeyfilePrefs(const BrotPrefs::KeyfilePrefs& src, BrotPrefs::KeyfilePrefs* parent);
};

#endif /* TESTPREFS_H_ */
