/*  b2test.cpp: brot2 basic test suite
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

#include "gtest/gtest.h"
#include <iostream>
#include <glibmm/keyfile.h>
#include <glibmm/init.h>

using namespace std;

class Brot2TestEnv : public ::testing::Environment {
public:
	virtual void SetUp() {
		Glib::init();
	}
};

int main(int argc, char **argv) {
	::testing::AddGlobalTestEnvironment(new Brot2TestEnv);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
