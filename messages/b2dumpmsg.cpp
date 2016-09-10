/*
    b2msgdump.cpp: Debug message dumper
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

#include <iostream>
#include <fstream>
#include <string>

#include "messages/brot2msgs.pb.h"
#include <google/protobuf/text_format.h>

void usage(char*argv0) {
	std::cerr << "Usage: " << argv0 << " [infile]" << std::endl;
	std::cerr << "This program dumps a brot2 message file." << std::endl;
}

int main (int argc, char**argv)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	b2msg::savefile file;

	switch(argc) {
		default:
			usage(argv[0]);
			return -1;
		case 1: // Read from stdin
			if (!file.ParsePartialFromIstream(&std::cin)) {
				std::cerr << "Failed to parse file." << std::endl;
				usage(argv[0]);
				return -1;
			}
			break;
		case 2: // Read from file
			std::fstream in(argv[1], std::ios::in | std::ios::binary);
			if (!file.ParsePartialFromIstream(&in)) {
				std::cerr << "Failed to parse file." << std::endl;
				usage(argv[0]);
				return -1;
			}
			break;
	}
	std::cout << file.DebugString() << std::endl;
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
