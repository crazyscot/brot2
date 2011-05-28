/*
    Fractal: Core fractal computation interface bits
    Copyright (C) 2010 Ross Younger

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

#include <string>
#include <map>
#include "Fractal.h"

const fvalue _consts::log2 = log(2.0);
const fvalue _consts::log3 = log(3.0);
const fvalue _consts::log4 = log(4.0);
const fvalue _consts::log5 = log(5.0);

FractalRegistry* FractalRegistry::_instance = 0;

/*
 * I assert that I don't need to worry about thread-safety, as
 * the first call happens during static constructor time -
 * before I set up any threads.
 */
std::map<std::string,Fractal*>& FractalRegistry::registry() {
	if (!_instance) {
		load_Mandelbrot();
		load_Mandelbar();
		_instance = new FractalRegistry();
	}
	return _instance->_registry;
}
