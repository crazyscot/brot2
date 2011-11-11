/*  prefstest2.cpp: Semantic test of Prefs working copy mechanism
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
#include "PrefsRegistry.h"
#include <iostream>
#include <glibmm/keyfile.h>
using namespace std;

int main(void)
{
	bool orig_b;
	try {
		const Prefs& p = Prefs::getMaster();

		{
			std::unique_ptr<Prefs> p2 = p.getWorkingCopy();
			MouseActions ma = p.mouseActions();
			p2->mouseActions(ma);

			try {
				std::unique_ptr<Prefs> p3 = p.getWorkingCopy(); // should fail
				cerr << "Unexpectedly able to make further working copy of Prefs!" << endl;
				return 1;
			} catch (Exception e) {
				// This is the expected case, ignore and carry on
			}

			try {
				std::unique_ptr<Prefs> p3 = p2->getWorkingCopy(); // should fail
				cerr << "Unexpectedly able to make child working copy of Prefs!" << endl;
				return 1;
			} catch (Exception e) {
				// This is the expected case, ignore and carry on
			}

			// Now, can we commit and reread correctly via the master?
			orig_b = p2->get(PREF(ShowControls));
			p2->set(PREF(ShowControls), !orig_b);
			p2->commit();

			bool readback = p.get(PREF(ShowControls));
			if (orig_b == readback) {
				cerr << "Writing preference did not work" << endl;
				return 1;
			}
		}

		// Now p2 has gone out of scope, this should work:
		{
			std::unique_ptr<Prefs> p4 = p.getWorkingCopy();
			p4->set(PREF(ShowControls), orig_b);
		}
		{
			// But it should not affect the master or a new working copy:
			bool readback = p.get(PREF(ShowControls));
			if (orig_b == readback) {
				cerr << "Uncommitted preference write stuck!" << endl;
				return 1;
			}
			// Now we set it back.
			std::unique_ptr<Prefs> p5 = p.getWorkingCopy();
			p5->set(PREF(ShowControls), orig_b);
			p5->commit();

			readback = p.get(PREF(ShowControls));
			if (orig_b != readback) {
				cerr << "Preference writeback did not stick!" << endl;
				return 1;
			}
		}

	} catch (Exception e) {
		cerr << "Error! " << e << endl;
		return 1;
	} catch (Glib::KeyFileError e) {
		cerr << "KeyFileError! " << e.what() << endl;
		return 1;
	}
	cout << "All OK." << endl;
	return 0;
}

