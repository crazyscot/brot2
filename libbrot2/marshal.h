/*
    marshal.h: Data struct conversion for marshalling
    Copyright (C) 2016 Ross Younger

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

#ifndef MARSHAL_H_
#define MARSHAL_H_

#include "libbrot2/MovieMode.h"
#include "messages/brot2msgs.pb.h"

namespace b2marsh {

bool runtime_type_check(void); // Sanity check, called by unit tests. If this returns false, marshalling code is broken.

// Type helpers
void Value2Wire(const Fractal::Value& val, b2msg::Float* wire);
void Wire2Value(const b2msg::Float& wire, Fractal::Value& val);

// Complex helpers
void Point2Wire(const Fractal::Point& pt, b2msg::Point* wire);
void Wire2Point(const b2msg::Point& wire, Fractal::Point& pt);

// Structure marshallers
void Movie2Wire(const struct Movie::MovieInfo& mov, b2msg::Movie* wire);
bool Wire2Movie(const b2msg::Movie& wire, struct Movie::MovieInfo& mov); // May return false if failed to look up fractal/palette
}; // b2marsh

#endif /* MARSHAL_H_ */
