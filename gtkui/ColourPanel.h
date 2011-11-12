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


#ifndef COLOURPANEL_H_
#define COLOURPANEL_H_

#include <gtkmm/drawingarea.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <cairomm/cairomm.h>

class ProddableLabel : public Gtk::Label {
	public:
		ProddableLabel();
		ProddableLabel(const Glib::ustring& label);
		virtual void prod() = 0;
};

class ColourPanel: public Gtk::DrawingArea {
	Gtk::Frame *_parent;
	Gdk::Color colour;
	ProddableLabel *pokeme;

public:
	ColourPanel(Gtk::Frame *parent, ProddableLabel *pokeit);
	virtual ~ColourPanel();

	void set_colour(const Gdk::Color& c);
	const Gdk::Color& get_colour() const;

    virtual bool on_button_release_event(GdkEventButton * evt);
    virtual bool on_expose_event(GdkEventExpose * evt);
    virtual bool on_configure_event(GdkEventConfigure * evt);
};

#endif /* COLOURPANEL_H_ */
