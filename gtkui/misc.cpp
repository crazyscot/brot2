/*
    misc.cpp: General miscellanea that didn't fit anywhere else
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

#include "misc.h"
#include <string>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int Util::copy_file(const std::string& from, const std::string& to) {
	int src = open(from.c_str(), O_RDONLY, 0);
	if (src==-1) return errno;
	int dest = open(to.c_str(), O_WRONLY|O_CREAT, 0644);
	if (dest==-1) return errno;

	struct stat stat1;
	fstat(src, &stat1);
	sendfile(dest, src, 0, stat1.st_size);
	if (close(src) == -1) return errno;
	if (close(dest) == -1) return errno;
	return 0;
}
