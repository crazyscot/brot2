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
#include "BaseHUD.h"
#include "HUD.h"
#include "MainWindow.h"
#include "Plot3Plot.h"
#include "Exception.h"
#include <pangomm.h>
#include <cairomm/cairomm.h>
#include <gdkmm/color.h>
#include <string>

using namespace BrotPrefs;

HUD::HUD(MainWindow &window, std::shared_ptr<const BrotPrefs::Prefs> _prefs) : parent(window), prefs(_prefs), last_drawn_w(0), last_drawn_h(0) {
}

// Must hold the lock before calling.
void HUD::ensure_surface_locked(const int rwidth, const int rheight)
{
	if ((rwidth!=last_drawn_w) || (rheight!=last_drawn_h)) {
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

void HUD::draw(Plot3::Plot3Plot* plot, const int rwidth, const int rheight) {
	std::unique_lock<std::mutex> lock(mux);
	ensure_surface_locked(rwidth, rheight);
	Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
	clear_locked(cr);

	BaseHUD::draw(surface, prefs, plot, rwidth, rheight, parent.is_at_max_zoom(), parent.is_at_min_zoom());

	last_drawn_w = rwidth;
	last_drawn_h = rheight;
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
