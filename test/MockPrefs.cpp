/*
    MockPrefs.cpp: Mocked preferences for testing
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

#include "MockPrefs.h"

using namespace BrotPrefs;

MockPrefs::MockPrefs() {
}

MockPrefs::~MockPrefs() {
}

#define UNSUPPORTED { THROW(PrefsException,"Unsupported"); }
std::shared_ptr<Prefs> MockPrefs::getWorkingCopy() const throw()
UNSUPPORTED;
void MockPrefs::commit() throw()
UNSUPPORTED;

const MouseActions& MockPrefs::mouseActions() const
UNSUPPORTED;
void MockPrefs::mouseActions(const MouseActions&)
UNSUPPORTED;

const ScrollActions& MockPrefs::scrollActions() const
UNSUPPORTED;
void MockPrefs::scrollActions(const ScrollActions&)
UNSUPPORTED;

void MockPrefs::set(const BrotPrefs::Numeric<int>&, int) UNSUPPORTED;
void MockPrefs::set(const BrotPrefs::Numeric<double>&, double) UNSUPPORTED;
void MockPrefs::set(const BrotPrefs::Boolean&, const bool) UNSUPPORTED;
void MockPrefs::set(const BrotPrefs::String&, const std::string&) UNSUPPORTED;

int MockPrefs::get(const BrotPrefs::Numeric<int>& B) const {
	if(B._name == "Minimum escapee %")
		return 20;
	if(B._name == "Initial maxiter")
		return 1;
	THROW(PrefsException,"Unknown "+B._name);
	return 0;
}

double MockPrefs::get(const BrotPrefs::Numeric<double>& B) const {
	if(B._name == "Live threshold")
		return 0.001;
	THROW(PrefsException,"Unknown "+B._name);
	return 0;
}
bool MockPrefs::get(const BrotPrefs::Boolean& B) const {
	THROW(PrefsException,"Unknown "+B._name);
	return false;
}

std::string MockPrefs::get(const BrotPrefs::String& B) const {
	THROW(PrefsException,"Unknown "+B._name);
	return "";
}
