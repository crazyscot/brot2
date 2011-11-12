/*
    PrefsRegistry.cpp: Master prefs list
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

#include <limits.h>
#include "PrefsRegistry.h"

namespace BrotPrefs {

	const std::string Groups::PLOT_CONTROL = "plot_control";
	const std::string Groups::UI = "ui";
	const std::string Groups::HUD = "hud";

	Registry Registry::_instance;

	const Registry& Registry::get() {
		return _instance;
	}

	Registry::Registry() :
		ShowControls("Show controls window",
				"Shows the controls window", // sigh
				true, Groups::UI, "show_controls"),

		InitialMaxIter("Initial maxiter",
				"First pass iteration limit (minimum 2)",
				2, 256, INT_MAX,
				Groups::PLOT_CONTROL, "initial_maxiter"),
		LiveThreshold("Live threshold",
				"Minimal pixel escape rate in order to consider a plot "
				"finished (0.0-1.0)",
				0.0, 0.001, 1.0,
				Groups::PLOT_CONTROL, "live_threshold"),
		MinEscapeePct("Minimum escapee %",
				"Percentage of pixels required to have escaped before a plot "
				"is considered finished",
				0, 20, 100,
				Groups::PLOT_CONTROL, "minimum_done_percent"),

		HUDVerticalOffset("HUD Vertical offset %",
				"HUD Vertical offset in % of window",
				0, 1, 100,
				Groups::HUD, "vertical_offset"),
		HUDHorizontalOffset("HUD Horizontal offset %",
				"HUD Horizontal offset in % of window",
				0, 1, 100,
				Groups::HUD, "horizontal_offset"),

		HUDText("HUD text colour",
				"HUD text colour (rrrrggggbbbb hex)",
				"#ffffffffffff",
				Groups::HUD, "text_colour"),
		HUDBackground("HUD bg colour",
				"HUD background colour (rrrrggggbbbb hex)",
				"#000000000000",
				Groups::HUD, "bg_colour"),

		end()
	{ }

}; // namespace BrotPrefs
