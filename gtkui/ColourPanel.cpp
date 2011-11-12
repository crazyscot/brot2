/*
    ColourPanel: Prefs panel which, when clicked, opens up a colour picker
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

#include "ColourPanel.h"
#include <iostream>
#include <gtkmm/colorselection.h>

ColourPanel::ColourPanel(Gtk::Frame *parent) : Gtk::DrawingArea(), _parent(parent) {
	set_size_request(50,50); // bare minimum, parent should FILL.
	add_events(Gdk::EXPOSURE_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::BUTTON_PRESS_MASK);
}
ColourPanel::~ColourPanel() {
}

void ColourPanel::set_colour(const Gdk::Color& c) {
	colour = c;
}

const Gdk::Color& ColourPanel::get_colour() const {
	return colour;
}

bool ColourPanel::on_button_release_event(GdkEventButton * evt)
{
	(void) evt;
	Gtk::ColorSelectionDialog dlg;
	dlg.get_color_selection()->set_current_color(colour);
	gint result = dlg.run();
	if (result == Gtk::ResponseType::RESPONSE_OK) {
		colour = dlg.get_color_selection()->get_current_color();
		//std::cerr << "new colour is " << colour.to_string() << std::endl; //TEST
		on_expose_event(0);
	}
	// ... AND prod parent.
	return true;
}

bool ColourPanel::on_expose_event(GdkEventExpose * evt)
{
	(void) evt;
	Glib::RefPtr<Gdk::Window> window = get_window();
	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();
	cr->set_source_rgb(colour.get_red()/255.0,
			colour.get_green()/255.0,
			colour.get_blue()/255.0);
	cr->paint();
	// An outer frame makes things seem more tidy:
	cr->set_source_rgb(0.0, 0.0, 0.0);
	cr->set_line_width(0.5);
	cr->rectangle(0, 0, get_width(), get_height());
	cr->stroke();
	
	return true;
}

bool ColourPanel::on_configure_event(GdkEventConfigure * evt)
{
	(void) evt;
	return true;
}

