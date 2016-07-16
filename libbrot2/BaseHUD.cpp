/*
    BaseHUD.cpp: Basic HUD functionality
    Copyright (C) 2011-6 Ross Younger

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

#include "BaseHUD.h"
#include "Prefs.h"
#include "Exception.h"
#include <pangomm/init.h>

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

const std::string BaseHUD::font_name = "sans-serif";

void BaseHUD::draw(Cairo::RefPtr<Cairo::Surface> surface, std::shared_ptr<const BrotPrefs::Prefs> prefs, Plot3::Plot3Plot* plot, const int rwidth, const int rheight, bool is_max, bool is_min)
{
	if (!plot) return; // race condition trap

	int xpos, ypos, xright, fontsize;
	Gdk::Color fg_gdk, bg_gdk;
	double alpha;

	std::string info = plot->info_zoom(prefs->get(PREF(HUDShowZoom)));
	if (is_max)
		info.append(" (max!)");
	if (is_min)
		info.append(" (min!)");

	BaseHUD::retrieve_prefs(prefs,fg_gdk,bg_gdk,alpha,xpos,ypos,xright,fontsize);
	const rgb_double fg(fg_gdk), bg(bg_gdk);
	const int hudwidthpct = MAX(xright - xpos, 1);

	const int XOFFSET = xpos * rwidth / 100;
	const int WIDTH_PIXELS = hudwidthpct * rwidth / 100;

	Pango::init();
	Pango::FontDescription fontdesc(BaseHUD::font_name);
	fontdesc.set_size(fontsize);
	fontdesc.set_weight(Pango::Weight::WEIGHT_BOLD);

	Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
	Glib::RefPtr<Pango::Layout> lyt = Pango::Layout::create(cr);
	lyt->set_font_description(fontdesc);
	lyt->set_markup(info);
	lyt->set_width(Pango::SCALE * WIDTH_PIXELS);
	lyt->set_wrap(Pango::WRAP_WORD_CHAR);

	// Make sure we fit.
	const int YOFFSET = ypos * (rheight - BaseHUD::compute_layout_height(lyt)) / 100;

	if (prefs->get(PREF(HUDOutlineText))) {
		// Outline text effect
		cr->save();
		cr->begin_new_path();
		cr->move_to(XOFFSET,YOFFSET);
		if (fontsize <= 13)
			cr->set_line_width(1.0);
		else
			cr->set_line_width(2.0);
		cr->set_operator(Cairo::Operator::OPERATOR_OVER);
		cr->set_source_rgba(bg.r, bg.g, bg.b, alpha);
		lyt->update_from_cairo_context(cr);
		lyt->add_to_cairo_context(cr);
		cr->stroke_preserve();
		lyt->show_in_cairo_context(cr);
		cr->restore();
		lyt->update_from_cairo_context(cr);
	} else {
		// Boring rectangular text background
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
	}

	// Finally, draw the text itself.
	cr->move_to(XOFFSET,YOFFSET);
	cr->set_operator(Cairo::Operator::OPERATOR_OVER);
	cr->set_source_rgba(fg.r, fg.g, fg.b, alpha);
	lyt->show_in_cairo_context(cr);
}

void BaseHUD::retrieve_prefs(std::shared_ptr<const Prefs> prefs,
		Gdk::Color& fgcol, Gdk::Color& bgcol, double& alpha,
		int& xpos, int& ypos, int& xright, int& fontsize)
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
	fontsize = Pango::SCALE * prefs->get(PREF(HUDFontSize));
}

unsigned BaseHUD::compute_layout_height(Glib::RefPtr<Pango::Layout> lyt)
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

void BaseHUD::apply(Render2::Base& target,
		std::shared_ptr<const BrotPrefs::Prefs> prefs,
		Plot3::Plot3Plot* plot,
		bool is_max,
		bool is_min)
{
	int rwidth = target.width(),
		rheight = target.height();
	// We'll use alpha in our surface because the HUD is partially transparent.
	Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, rwidth, rheight);
	// N.B. Cairo::ImageSurface docs say the new surface is initialised to (0,0,0,0).

	BaseHUD::draw(surface, prefs, plot, rwidth, rheight, is_max, is_min);

	surface->flush();
	unsigned char * image_data = surface->get_data();
	ASSERT(surface->get_stride() > 0);
	ASSERT(surface->get_height() == rheight);
	ASSERT(surface->get_width() == rwidth);
	unsigned char *rowptr = image_data;
	for (unsigned y = 0; y<rheight; y++) {
		unsigned char *src = rowptr;
		rgb current;
		for (unsigned x = 0; x<rwidth; x++) {
			// This is hard-coded for FORMAT_ARGB32.
			// TODO: Cairo stores native-endian. Fix these to be endian-safe.
			unsigned alpha = src[3];
			if (alpha != 0) {
				target.pixel_get(x, y, current);
				rgba overpix(src[2], src[1], src[0], alpha);
				current.overlay(overpix);
				target.pixel_done(x, y, current);
			}
			src+=4;
		}
		rowptr += surface->get_stride();
	}
}
