/*
    MockPrefs.h: Mocked preferences for testing
    Copyright (C) 2012 Ross Younger

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

#ifndef MOCKPREFS_H_
#define MOCKPREFS_H_

#include "Prefs.h"

class MockPrefs: public Prefs {
public:
	MockPrefs();
	virtual ~MockPrefs();

	virtual std::shared_ptr<Prefs> getWorkingCopy() const throw(Exception);
	virtual void commit() throw(Exception);

	virtual const MouseActions& mouseActions() const;
	virtual void mouseActions(const MouseActions& mouse);

	virtual const ScrollActions& scrollActions() const;
	virtual void scrollActions(const ScrollActions& scroll);

	virtual int get(const BrotPrefs::Numeric<int>& B) const;
	virtual void set(const BrotPrefs::Numeric<int>& B, int newval);
	virtual double get(const BrotPrefs::Numeric<double>& B) const;
	virtual void set(const BrotPrefs::Numeric<double>& B, double newval);
	virtual bool get(const BrotPrefs::Boolean& B) const;
	virtual void set(const BrotPrefs::Boolean& B, const bool newval);

	virtual std::string get(const BrotPrefs::String& B) const;
	virtual void set(const BrotPrefs::String& B, const std::string& newval);
};

#endif /* MOCKPREFS_H_ */
