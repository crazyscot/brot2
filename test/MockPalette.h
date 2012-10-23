/*
    MockPalette.h: For unit testing
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

#ifndef MOCKPALETTE_H_
#define MOCKPALETTE_H_

#include "palette.h"

class MockPalette: public BasePalette {
public:
	MockPalette() {}
	virtual rgb get(const Fractal::PointData &) const { return rgb(255,255,255); }
	virtual ~MockPalette() {}
};

#endif /* MOCKPALETTE_H_ */
