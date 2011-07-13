/*
    SaveAsPNG: PNG export action/support for brot2
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

#include <png.h>

#include "SaveAsPNG.h"
#include "misc.h"

#include "MainWindow.h"
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/messagedialog.h>
#include <iostream>

#include <errno.h>
#include <stdio.h>
#include <string.h>

std::string SaveAsPNG::last_saved_dirname = "";

void SaveAsPNG::to_png(MainWindow *mw, std::string& filename)
{
	FILE *f;

	f = fopen(filename.c_str(), "wb");
	if (f==NULL) {
		std::ostringstream str;
		str << "Could not open file for writing: " << errno << " (" << strerror(errno) << ")";
		Util::alert(mw, str.str());
		return;
	}

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop png_info=0;
	if (png)
		png_info = png_create_info_struct(png);
	if (!png_info) {
		Util::alert(mw, "Could not create PNG structs (out of memory?)");
		if (png)
			png_destroy_write_struct(&png, 0);
		return;
	}
	jmp_buf jbuf;
	if (setjmp(jbuf)) {
		fclose(f);
		return;
	}

	const unsigned width = mw->get_rwidth(),
			height = mw->get_rheight();

	png_init_io(png, f);

	png_set_compression_level(png, Z_BEST_SPEED);
	png_set_IHDR(png, png_info,
			width, height, 8 /* 24bpp */,
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	std::string comment = mw->get_plot().info(true);
	const char* SOFTWARE = "brot2",
		      * INFO = comment.c_str();
	png_text texts[2] = {
			{PNG_TEXT_COMPRESSION_NONE, (char*)"Software", (char*)SOFTWARE, strlen(SOFTWARE)},
			{PNG_TEXT_COMPRESSION_NONE, (char*)"Comment", (char*)INFO, strlen(INFO) },
	};
	png_set_text(png, png_info, texts, 2);

	png_write_info(png, png_info);

	const int rowstride = MainWindow::RGB_BYTES_PER_PIXEL * width;

	guchar * pngbuf = new unsigned char[rowstride * height];
	mw->render_generic(pngbuf, rowstride, -1, pixpack_format::PACKED_RGB_24);
	guchar ** pngrows = new unsigned char*[height];
	for (unsigned i=0; i<height; i++) {
		pngrows[i] = pngbuf + rowstride*i;
	}
	png_write_image(png, pngrows);
	delete[] pngrows;
	delete[] pngbuf;
	png_write_end(png,png_info);
	png_destroy_write_struct(&png, &png_info);

	if (0==fclose(f)) {
		mw->get_progbar()->set_text("Successfully saved");
	} else {
		std::ostringstream str;
		str << "Error closing the save: " << errno << " (" << strerror(errno) << ")";
		Util::alert(mw, str.str());
	}
}

void SaveAsPNG::do_save(MainWindow *mw)
{
	Gtk::FileChooserDialog dialog(*mw, "Save File", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
	dialog.set_do_overwrite_confirmation(true);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::ResponseType::RESPONSE_ACCEPT);

	if (!last_saved_dirname.length()) {
		std::ostringstream str;
		str << "/home/" << getenv("USERNAME") << "/Documents/";
		dialog.set_current_folder(str.str().c_str());
	} else {
		dialog.set_current_folder(last_saved_dirname.c_str());
	}

	{
		std::ostringstream str;
		str << mw->get_plot().info().c_str() << ".png";
		dialog.set_current_name(str.str().c_str());
	}
	int rv = dialog.run();
	if (rv != GTK_RESPONSE_ACCEPT) return;

	std::string filename = dialog.get_filename();
	last_saved_dirname.clear();
	last_saved_dirname.append(filename);
	// Trim the filename to leave the dir:
	int spos = last_saved_dirname.rfind('/');
	if (spos >= 0)
		last_saved_dirname.erase(spos+1);

	to_png(mw, filename);
}
