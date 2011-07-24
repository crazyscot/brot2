/*
    registry.h: Template-driven class registry
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

#ifndef REGISTRY_H_
#define REGISTRY_H_

#include <string>
#include <set>
#include <map>

template <class T>
class RegistryWithoutDescription
{
	protected:
		std::map<std::string, T*> instances;
	public:
		// Register an instance and its description
		void reg(const std::string& name, T* it) {
			instances[name] = it;
		}

		// Retrieve an instance
		T* get(const std::string& name) {
			return instances[name];
		}

		// Computes and returns the set of names. Potentially expensive,
		// so don't overuse it.
		std::set<std::string> names() const {
			std::set<std::string> rset;
			typename std::map<std::string,T*>::const_iterator it = instances.begin();
			for ( ; it != instances.end(); it++ )
				rset.insert(it->first);
			return rset;
		}

		// Deregister an instance
		void dereg(const std::string& name) {
			instances.erase(name);
		}
};

#endif /* REGISTRY_H_ */
