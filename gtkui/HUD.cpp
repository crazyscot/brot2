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

HUD::HUD(MainWindow &window) : parent(window), w(0), h(0) {
}

void HUD::draw(Plot2* plot, const int rwidth, const int rheight)
{
	Glib::Mutex::Lock autolock(mux); // unwind unlocks
	if (!plot) return; // race condition trap
	std::string info = plot->info(true);

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

	Glib::RefPtr<Pango::Layout> lyt = Pango::Layout::create(cr);
	Pango::FontDescription fontdesc("Luxi Sans 9");
	lyt->set_font_description(fontdesc);
	lyt->set_text(info);
	lyt->set_width(Pango::SCALE * rwidth);
	lyt->set_wrap(Pango::WRAP_WORD_CHAR);

	//Pango::Rectangle rect = lyt->get_logical_extents();

	Pango::LayoutIter iter = lyt->get_iter();
	do {
		Pango::Rectangle log = iter.get_line_logical_extents();
		cr->save();
		cr->set_source_rgb(0,0,0);
		// NB. there are PANGO_SCALE pango units to the device unit.
		cr->rectangle(log.get_x() / PANGO_SCALE, log.get_y() / PANGO_SCALE,
				log.get_width() / PANGO_SCALE, log.get_height() / PANGO_SCALE);
		cr->clip();
		cr->paint();
		cr->restore();
	} while (iter.next_line());

	cr->move_to(0,0);
	cr->set_operator(Cairo::Operator::OPERATOR_OVER);
	cr->set_source_rgb(1.0,1.0,1.0);
	lyt->show_in_cairo_context(cr);
}

void HUD::clear_locked(Cairo::RefPtr<Cairo::Context> cr) {
	cr->save();
	cr->set_source_rgba(0,0,0,0);
	cr->set_operator(Cairo::Operator::OPERATOR_SOURCE);
	cr->paint();
	cr->restore();
}
