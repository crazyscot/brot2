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

#include "gtest/gtest.h"

#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <iostream>

#include "PrefsT.h"
#include "PrefsRegistry.h"

using namespace BrotPrefs;

TestingKeyfilePrefs::TestingKeyfilePrefs() : KeyfilePrefs() {
}

TestingKeyfilePrefs* TestingKeyfilePrefs::getInstance() {
	TestingKeyfilePrefs* rv = new TestingKeyfilePrefs();
	rv->initialise();
	return rv;
}

TestingKeyfilePrefs::TestingKeyfilePrefs(const KeyfilePrefs& src, KeyfilePrefs* parent) : KeyfilePrefs(src,parent) {
}

TestingKeyfilePrefs* TestingKeyfilePrefs::getInstance(const KeyfilePrefs& src, KeyfilePrefs* parent) {
	TestingKeyfilePrefs* rv = new TestingKeyfilePrefs(src,parent);
	rv->initialise();
	return rv;
}

TestingKeyfilePrefs::~TestingKeyfilePrefs() {
	if (_parent==NULL)
		unlink(filename().c_str());
}

std::string TestingKeyfilePrefs::filename(bool temp) {
	std::ostringstream rv;
	char *tmpdir = getenv("TMP");
	if (tmpdir==NULL)
		rv << "/tmp/";
	else
		rv << tmpdir << '/';
	rv << "brot2.tempprefs." << getpid();
	if (temp)
		rv << ".tmp";
	return rv.str();
}

std::shared_ptr<Prefs> TestingKeyfilePrefs::getWorkingCopy() const throw(){
	if (_parent != NULL)
		ADD_FAILURE() << "Prefs: Cannot make a working copy of a working copy!";
	if (_childCount)
		ADD_FAILURE() << "Prefs: cannot make a working copy when there's one outstanding";

	Prefs *rv = getInstance(*this, const_cast<TestingKeyfilePrefs*>(this));
	std::shared_ptr<Prefs> prv(rv);
	return prv;
}

TEST(prefs,ReadOutAllItems) {
	TestingKeyfilePrefs* pm = TestingKeyfilePrefs::getInstance();
	std::shared_ptr<Prefs> p = pm->getWorkingCopy();
	MouseActions ma = p->mouseActions();
	p->mouseActions(ma);
	ScrollActions sa = p->scrollActions();
	p->scrollActions(sa);
	//p->commit();
	for (int i=1; i<=ma.MAX; i++)
		ma[i];
	//cout << "action "<<i<<" is " << (std::string) ma[i] << endl;

#define DO(type,name) p->get(PREF(name));
	ALL_PREFS(DO);

	delete pm;
}

TEST(prefs,ThereCanBeOnlyOne) {
	std::shared_ptr<const Prefs> p = Prefs::getMaster();
	std::shared_ptr<Prefs> p2 = p->getWorkingCopy();
	MouseActions ma = p->mouseActions();
	p2->mouseActions(ma);
	// Cannot make a working copy while there is one outstanding:
	EXPECT_THROW(std::shared_ptr<Prefs> p3 = p->getWorkingCopy(), PrefsException);
	// Cannot make a child working copy of a working copy:
	EXPECT_THROW(std::shared_ptr<Prefs> p3 = p2->getWorkingCopy(), PrefsException);
}

TEST(prefs,CommitAffectsMasterAndSubsequentChildren) {
	TestingKeyfilePrefs* pm = TestingKeyfilePrefs::getInstance();
	for (int pct=1; pct<100; pct+=42) {
		{
			std::shared_ptr<Prefs> p = pm->getWorkingCopy();
			p->set(PREF(MinEscapeePct), pct);
			p->commit();
		}
		EXPECT_EQ(pct, pm->get(PREF(MinEscapeePct)));
		std::shared_ptr<Prefs> p2 = pm->getWorkingCopy();
		EXPECT_EQ(pct, p2->get(PREF(MinEscapeePct)));
	}
	// And the same thing for bools:
	bool orig_b;
	{
		std::shared_ptr<Prefs> p2 = pm->getWorkingCopy();
		orig_b = p2->get(PREF(ShowControls));
		p2->set(PREF(ShowControls), !orig_b);
		p2->commit();
	}
	EXPECT_EQ(!orig_b, pm->get(PREF(ShowControls)));
	{
		std::shared_ptr<Prefs> p3 = pm->getWorkingCopy();
		EXPECT_EQ(!orig_b, p3->get(PREF(ShowControls)));
	}
	delete pm;
}

TEST(prefs, UncommittedChangesDoNotPropagate) {
	TestingKeyfilePrefs* pm = TestingKeyfilePrefs::getInstance();
	bool orig_b;
	{
		std::shared_ptr<Prefs> p4 = pm->getWorkingCopy();
		orig_b = p4->get(PREF(ShowControls));
		p4->set(PREF(ShowControls), !orig_b);
		EXPECT_EQ(orig_b, pm->get(PREF(ShowControls)));
	}
	std::shared_ptr<Prefs> p5 = pm->getWorkingCopy();
	EXPECT_EQ(orig_b, pm->get(PREF(ShowControls)));
	delete pm;
}
