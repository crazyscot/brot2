/*
    HUD: Heads-Up Display for brot2
    Copyright (C) 2010-2011 Ross Younger

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

#include "HUD.h"
#include "MainWindow.h"
#include "Plot2.h"
#include "Exception.h"
#include <pangomm.h>
#include <cairomm/cairomm.h>
#include <string>

struct rgb_double {
	double r, g, b;
	rgb_double() { }
	rgb_double(Gdk::Color c) {
		r = c.get_red()/65535.0;
		g = c.get_green()/65535.0;
		b = c.get_blue()/65535.0;
	}
};

HUD::HUD(MainWindow &window) : parent(window), w(0), h(0) {
}

const std::string HUD::font_name = "Luxi Sans 9";

void HUD::draw(Plot2* plot, const int rwidth, const int rheight)
{
	Glib::Mutex::Lock autolock(mux); // unwind unlocks
	if (!plot) return; // race condition trap
	std::string info = plot->info(true);
	const Prefs& prefs = parent.prefs();
	const int ypos = prefs.get(PREF(HUDVerticalOffset)),
		  xpos = prefs.get(PREF(HUDHorizontalOffset));

	rgb_double fg, bg;
	{
		Gdk::Color fgcol, bgcol;
		const std::string fgtext = prefs.get(PREF(HUDText)),
			  bgtext = prefs.get(PREF(HUDBackground));
		if (!fgcol.set(fgtext))
			fgcol.set(PREF(HUDText)._default);
		if (!bgcol.set(bgtext))
			bgcol.set(PREF(HUDBackground)._default);
		fg = fgcol;
		bg = bgcol;
	}
	const double alpha = 1.0 - prefs.get(PREF(HUDTransparency));

	if ((rwidth!=w) || (rheight!=h)) {
		if (surface)
			surface->finish();
		surface.clear();
		ASSERT(!surface);
	}

	if (!surface) {
		Glib::RefPtr<Gdk::Window> w = parent.get_window();
		surface = w->create_similar_surface(Cairo::CONTENT_COLOR_ALPHA, rwidth, rheight);
	}

	Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
	clear_locked(cr);

	w = rwidth;
	h = rheight;

	const int XOFFSET = xpos * rwidth / 100;

	Glib::RefPtr<Pango::Layout> lyt = Pango::Layout::create(cr);
	Pango::FontDescription fontdesc(font_name);
	lyt->set_font_description(fontdesc);
	lyt->set_text(info);
	lyt->set_width(Pango::SCALE * (rwidth - XOFFSET));
	lyt->set_wrap(Pango::WRAP_WORD_CHAR);

	// add up the height, make sure we fit.
	Pango::LayoutIter iter = lyt->get_iter();
	int ytotal = 0;
	do {
		int yy = iter.get_line_logical_extents().get_height();
		ytotal += yy;
	} while (iter.next_line());
	ytotal /= PANGO_SCALE;

	const int YOFFSET = ypos * (rheight - ytotal) / 100;

	//Pango::Rectangle rect = lyt->get_logical_extents();

	// Now iterate again for the text background.
	iter = lyt->get_iter();
	do {
		Pango::Rectangle log = iter.get_line_logical_extents();
		cr->save();
		cr->set_source_rgb(bg.r, bg.g, bg.b);
		// NB. there are PANGO_SCALE pango units to the device unit.
		cr->rectangle(XOFFSET + log.get_x() / PANGO_SCALE,
				YOFFSET + log.get_y() / PANGO_SCALE,
				log.get_width() / PANGO_SCALE, log.get_height() / PANGO_SCALE);
		cr->clip();
		cr->paint_with_alpha(alpha);
		cr->restore();
	} while (iter.next_line());

	cr->move_to(XOFFSET,YOFFSET);
	cr->set_operator(Cairo::Operator::OPERATOR_OVER);
	cr->set_source_rgba(fg.r, fg.g, fg.b, alpha);
	lyt->show_in_cairo_context(cr);
}

void HUD::clear_locked(Cairo::RefPtr<Cairo::Context> cr) {
	cr->save();
	cr->set_source_rgba(0,0,0,0);
	cr->set_operator(Cairo::Operator::OPERATOR_SOURCE);
	cr->paint();
	cr->restore();
}
