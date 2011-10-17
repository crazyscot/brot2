/*  prefstest.cpp: Test program for prefs lib
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

#include "Prefs.h"
#include <iostream>
#include <glibmm/keyfile.h>
using namespace std;

int main(void)
{
	int i;
	try {
		Prefs& p = Prefs::getDefaultInstance();
		MouseActions ma = p.mouseActions();
		p.mouseActions(ma);
		//ScrollActions sa = p.scrollActions();
		//p.scrollActions(sa);
		p.commit();
		for (i=1; i<=ma.MAX; i++)
			cout << "action "<<i<<" is " << (std::string) ma[i] << endl;
	} catch (Exception e) {
		cerr << "Error! " << e << endl;
		return 1;
	} catch (Glib::KeyFileError e) {
		cerr << "KeyFileError! " << e.what() << endl;
		return 1;
	}
	return 0;
}

