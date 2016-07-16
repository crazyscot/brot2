/*
    BaseHUD.h: Basic HUD functionality
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

#ifndef BASEHUD_H_
#define BASEHUD_H_

#include "Prefs.h"
#include "Plot3Plot.h"
#include "Render2.h"
#include <memory>
#include <cairomm/cairomm.h>
#include <gdkmm/color.h>
#include <pangomm.h>

class BaseHUD {
	BaseHUD(BaseHUD&) = delete;
	const BaseHUD& operator=( const BaseHUD& ) = delete;

	public:
	static const std::string font_name;

	static void retrieve_prefs(std::shared_ptr<const BrotPrefs::Prefs> prefs,
			Gdk::Color& fg, Gdk::Color& bg, double& alpha,
			int& xpos, int& ypos, int& xright, int& fontsize);

	static unsigned compute_layout_height(Glib::RefPtr<Pango::Layout> lyt);

	/* Draws the HUD onto the given surface */
	static void draw(Cairo::RefPtr<Cairo::Surface> surface,
			std::shared_ptr<const BrotPrefs::Prefs> prefs,
			Plot3::Plot3Plot* plot,
			const int rwidth,
			const int rheight,
			bool is_max,
			bool is_min);

	/* Draws the given HUD onto a dummy surface then applies it to the given target renderer */
	static void apply(Render2::Base& target,
			std::shared_ptr<const BrotPrefs::Prefs> prefs,
			Plot3::Plot3Plot* plot,
			bool is_max,
			bool is_min);
};
#endif /* BASEHUD_H_ */
