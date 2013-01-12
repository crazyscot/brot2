/*
    HUD: Heads-Up Display for brot2
    Copyright (C) 2010-2012 Ross Younger

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

#include "png.h" // see launchpad 218409
#include "HUD.h"
#include "MainWindow.h"
#include "Plot3Plot.h"
#include "Exception.h"
#include <pangomm.h>
#include <cairomm/cairomm.h>
#include <gdkmm/color.h>
#include <string>

using namespace BrotPrefs;

struct rgb_double {
	double r, g, b;
	rgb_double() : r(0), g(0), b(0) { }
	rgb_double(Gdk::Color c) {
		r = c.get_red()/65535.0;
		g = c.get_green()/65535.0;
		b = c.get_blue()/65535.0;
	}
};

HUD::HUD(MainWindow &window) : parent(window), w(0), h(0) {
}

const std::string HUD::font_name = "Luxi Sans 9";

void HUD::retrieve_prefs(std::shared_ptr<const Prefs> prefs,
		Gdk::Color& fgcol, Gdk::Color& bgcol, double& alpha,
		int& xpos, int& ypos, int& xright)
{
	const std::string fgtext = prefs->get(PREF(HUDTextColour)),
		  bgtext = prefs->get(PREF(HUDBackgroundColour));
	if (!fgcol.set(fgtext))
		fgcol.set(PREF(HUDTextColour)._default);
	if (!bgcol.set(bgtext))
		bgcol.set(PREF(HUDBackgroundColour)._default);

	alpha = 1.0 - prefs->get(PREF(HUDTransparency));

	xpos = prefs->get(PREF(HUDHorizontalOffset));
	ypos = prefs->get(PREF(HUDVerticalOffset));
	xright = prefs->get(PREF(HUDRightMargin));
}

// Must hold the lock before calling.
void HUD::ensure_surface_locked(const int rwidth, const int rheight)
{
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
}

unsigned HUD::compute_layout_height(Glib::RefPtr<Pango::Layout> lyt)
{
	Pango::LayoutIter iter = lyt->get_iter();
	unsigned ytotal = 0;
	do {
		int yy = iter.get_line_logical_extents().get_height();
		ytotal += yy;
	} while (iter.next_line());
	ytotal /= PANGO_SCALE;
	return ytotal;
}

void HUD::draw(Plot3::Plot3Plot* plot, const int rwidth, const int rheight)
{
	std::unique_lock<std::mutex> lock(mux);
	if (!plot) return; // race condition trap
	std::string info = plot->info(true);
	int xpos, ypos, xright;
	Gdk::Color fg_gdk, bg_gdk;
	double alpha;

	retrieve_prefs(parent.prefs(),fg_gdk,bg_gdk,alpha,xpos,ypos,xright);
	const rgb_double fg(fg_gdk), bg(bg_gdk);
	const int hudwidthpct = MAX(xright - xpos, 1);

	ensure_surface_locked(rwidth, rheight);

	Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
	clear_locked(cr);

	w = rwidth;
	h = rheight;

	const int XOFFSET = xpos * rwidth / 100;
	const int WIDTH_PIXELS = hudwidthpct * rwidth / 100;

	Glib::RefPtr<Pango::Layout> lyt = Pango::Layout::create(cr);
	Pango::FontDescription fontdesc(font_name);
	lyt->set_font_description(fontdesc);
	lyt->set_text(info);
	lyt->set_width(Pango::SCALE * WIDTH_PIXELS);
	lyt->set_wrap(Pango::WRAP_WORD_CHAR);

	// Make sure we fit.
	const int YOFFSET = ypos * (rheight - compute_layout_height(lyt)) / 100;

	// Now paint the text background.
	Pango::LayoutIter iter = lyt->get_iter();
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

	// Finally, draw the text itself.
	cr->move_to(XOFFSET,YOFFSET);
	cr->set_operator(Cairo::Operator::OPERATOR_OVER);
	cr->set_source_rgba(fg.r, fg.g, fg.b, alpha);
	lyt->show_in_cairo_context(cr);
}

// Must hold the lock before calling.
void HUD::clear_locked(Cairo::RefPtr<Cairo::Context> cr) {
	cr->save();
	cr->set_source_rgba(0,0,0,0);
	cr->set_operator(Cairo::Operator::OPERATOR_SOURCE);
	cr->paint();
	cr->restore();
}

void HUD::erase() {
	if (!surface) return; // Nothing to do
	Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
	clear_locked(cr);
}
