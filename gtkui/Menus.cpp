/*
    Menus: menu bar for brot2
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

#include "Fractal.h"
#include "Menus.h"
#include "MainWindow.h"

#include <gtkmm/main.h>
#include <gtkmm/menu.h>
#include <gtkmm/stock.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/accelmap.h>
#include <gtkmm/accelgroup.h>

#include <gdkmm/pixbuf.h>

#include "config.h"
#include "gtkmain.h"
#include "logo.h" // in libbrot2
#include "MainWindow.h"
#include "ParamsDialog.h"
#include "SaveAsPNG.h"

namespace menus {

static MainWindow* find_main(Gtk::Menu *mnu) {
	if (!mnu) return 0;
    MainWindow *parent = 0;

    Gtk::Widget *attached = mnu->get_attach_widget();
	if (!attached) return 0;
    Gtk::Container *w = attached->get_parent();
    while(w && (parent = dynamic_cast<MainWindow *>(w)) == NULL)
      w = w->get_parent();
    return parent;
}

class MainMenu : public Gtk::Menu {
public:
	Gtk::ImageMenuItem aboutI, saveI, quitI;
	Gtk::SeparatorMenuItem sepa;

	MainMenu() : aboutI(Gtk::Stock::ABOUT), saveI(Gtk::Stock::SAVE), quitI(Gtk::Stock::QUIT) {
		append(aboutI);
		aboutI.signal_activate().connect(sigc::ptr_fun(do_about));
		saveI.set_label("_Save image...");
		append(saveI);
		saveI.signal_activate().connect(sigc::mem_fun(this, &MainMenu::do_save));
		append(sepa);
		append(quitI);
		quitI.signal_activate().connect(sigc::ptr_fun(do_quit));
	}

	static void do_about() {
		Glib::RefPtr<Gdk::Pixbuf> logo = Gdk::Pixbuf::create_from_inline(-1, brot2_logo, false);
		Gtk::AboutDialog dlg;
		dlg.set_version(PACKAGE_VERSION);
		dlg.set_comments("Dedicated to the memory of Benoît B. Mandelbrot.");
		dlg.set_copyright(copyright_string);
		dlg.set_license(license_text);
		dlg.set_wrap_license(true);
		dlg.set_logo(logo);
		dlg.run();
	}
	static void do_quit() {
		Gtk::Main::instance()->quit();
	}
	void do_save() {
		MainWindow *mw = find_main(this);
		SaveAsPNG::do_save(mw);
	}
};

class PlotMenu : public Gtk::Menu {
public:
	Gtk::MenuItem Undo;
	Gtk::ImageMenuItem Params, ZoomIn, ZoomOut, Stop, Redraw, More;
	Gtk::SeparatorMenuItem Sepa1, Sepa2;

	PlotMenu(MainWindow& parent) : Undo("_Undo", true),
			Params(Gtk::Stock::PROPERTIES),
			ZoomIn(Gtk::Stock::ZOOM_IN), ZoomOut(Gtk::Stock::ZOOM_OUT),
			Stop(Gtk::Stock::STOP), Redraw(Gtk::Stock::REFRESH), More(Gtk::Stock::EXECUTE)
	{
		Glib::RefPtr<Gtk::AccelGroup> ag = Gtk::AccelGroup::create();
		set_accel_group(ag);
		parent.add_accel_group(ag);

		append(Undo);
		Undo.signal_activate().connect(sigc::mem_fun(this, &PlotMenu::do_undo));
		Undo.add_accelerator("activate", ag, GDK_Z, Gdk::ModifierType::CONTROL_MASK, Gtk::ACCEL_VISIBLE);

		append(Params);
		Params.add_accelerator("activate", ag, GDK_P, Gdk::ModifierType::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
		Params.signal_activate().connect(sigc::mem_fun(this, &PlotMenu::do_params));

		append(Sepa1);
		append(ZoomIn);
		ZoomIn.add_accelerator("activate", ag, GDK_plus, Gdk::ModifierType::SHIFT_MASK, Gtk::ACCEL_VISIBLE);
		ZoomIn.signal_activate().connect(sigc::bind<MainWindow::Zoom>(
				sigc::mem_fun(this, &PlotMenu::do_zoom), MainWindow::Zoom::ZOOM_IN));

		append(ZoomOut);
		ZoomOut.add_accelerator("activate", ag, GDK_minus, Gdk::ModifierType::SHIFT_MASK, Gtk::ACCEL_VISIBLE);
		ZoomOut.signal_activate().connect(sigc::bind<MainWindow::Zoom>(
				sigc::mem_fun(this, &PlotMenu::do_zoom), MainWindow::Zoom::ZOOM_OUT));

		append(Sepa2);
		append(Stop);
		Stop.add_accelerator("activate", ag, GDK_period, Gdk::ModifierType::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
		Stop.signal_activate().connect(sigc::mem_fun(this, &PlotMenu::do_stop));

		append(Redraw);
		Redraw.set_label("_Redraw");
		Redraw.add_accelerator("activate", ag, GDK_R, Gdk::ModifierType::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
		Redraw.signal_activate().connect(sigc::mem_fun(this, &PlotMenu::do_redraw));

		append(More);
		More.set_label("_More iterations");
		More.add_accelerator("activate", ag, GDK_M, Gdk::ModifierType::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
		More.signal_activate().connect(sigc::mem_fun(this, &PlotMenu::do_more));
	}

	void do_undo() {
		MainWindow *mw = find_main(this);
		mw->do_undo();
	}
	void do_params() {
		MainWindow *mw = find_main(this);
		ParamsDialog params(mw);
		int rv = params.run();
		if (rv == Gtk::ResponseType::RESPONSE_ACCEPT)
			mw->do_plot();
	}
	void do_zoom(MainWindow::Zoom z) {
		MainWindow *mw = find_main(this);
		mw->do_zoom(z);
	}
	void do_stop() {
		MainWindow *mw = find_main(this);
		mw->do_stop();
	}
	void do_redraw() {
		MainWindow *mw = find_main(this);
		mw->do_plot(true);
	}
	void do_more() {
		MainWindow *mw = find_main(this);
		mw->do_more_iters();
	}

};

class OptionsMenu : public Gtk::Menu {
public:
	Gtk::CheckMenuItem drawHUD, antiAlias;

	OptionsMenu(MainWindow& parent) : drawHUD("Draw _HUD", true), antiAlias("_Antialias", true) {
		Glib::RefPtr<Gtk::AccelGroup> ag = Gtk::AccelGroup::create();
		set_accel_group(ag);
		parent.add_accel_group(ag);

		append(drawHUD);
		drawHUD.set_active(true);
		drawHUD.signal_toggled().connect(sigc::mem_fun(this, &OptionsMenu::toggle_drawHUD));
		drawHUD.add_accelerator("activate", ag, GDK_H, Gdk::ModifierType::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
		append(antiAlias);
		antiAlias.signal_toggled().connect(sigc::mem_fun(this, &OptionsMenu::toggle_antialias));
		antiAlias.add_accelerator("activate", ag, GDK_A, Gdk::ModifierType::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
	}
	void toggle_drawHUD() {
		MainWindow *mw = find_main(this);
		mw->toggle_hud();
	}
	void toggle_antialias() {
		MainWindow *mw = find_main(this);
		mw->toggle_antialias();
	}
};

class FractalMenu : public Gtk::Menu {
	MainWindow* _parent;
public:
	FractalMenu(MainWindow& parent, std::string& initial) {
		Fractal::FractalCommon::load_base();

		_parent = &parent;
		Gtk::RadioButtonGroup group;

		std::set<std::string> fractals = Fractal::FractalCommon::registry.names();
		std::set<std::string>::iterator it;
		unsigned maxsortorder = 0;
		for (it = fractals.begin(); it != fractals.end(); it++) {
			Fractal::FractalImpl *f = Fractal::FractalCommon::registry.get(*it);
			if (f->sortorder > maxsortorder)
				maxsortorder = f->sortorder;
		}

		unsigned sortpass=0;
		for (sortpass=0; sortpass <= maxsortorder; sortpass++) {
			for (it = fractals.begin(); it != fractals.end(); it++) {
				Fractal::FractalImpl *f = Fractal::FractalCommon::registry.get(*it);
				if (f->sortorder != sortpass)
					continue;

				Gtk::RadioMenuItem *item = new Gtk::RadioMenuItem(group, it->c_str());
				append(*manage(item));

				if (0==strcmp(initial.c_str(),f->name.c_str())) {
					item->set_active(true);
				}
				item->signal_activate().connect(
						sigc::bind<Gtk::RadioMenuItem*>(
							sigc::mem_fun(*this, &FractalMenu::selection),
							item));
				item->set_tooltip_text(f->description.c_str());
		}
	}

		if (!Fractal::FractalCommon::registry.get(initial)) {
			std::cerr << "FATAL: Initial fractal selection " << initial << " not found. Link error?" << std::endl;
			abort();
		}
		selection1(initial);
	}
	void selection(Gtk::RadioMenuItem *item) {
		selection1(item->get_label());
	}
	void selection1(const std::string& lbl) {
		Fractal::FractalImpl *sel = Fractal::FractalCommon::registry.get(lbl);
		if (sel) {
			_parent->fractal=sel;
			_parent->do_plot(false);
		}
	}
};

class ColourMenu : public Gtk::Menu {
	MainWindow* _parent;
public:
	ColourMenu(MainWindow& parent, std::string& initial) {
		_parent = &parent;
		Gtk::RadioButtonGroup group;
		bool got_init = false;

		Gtk::MenuItem *i1 = new Gtk::MenuItem("Discrete");
		i1->set_sensitive(false);
		append(*manage(i1));

		DiscretePalette::register_base();
		std::set<std::string> discretes = DiscretePalette::all.names();
		std::set<std::string>::iterator it;
		for (it = discretes.begin(); it != discretes.end(); it++) {
			Gtk::RadioMenuItem *item = new Gtk::RadioMenuItem(group, it->c_str());
			append(*manage(item));

			if (0==strcmp(initial.c_str(),it->c_str())) {
				item->set_active(true);
				got_init = true;
			}
			item->signal_activate().connect(
					sigc::bind<Gtk::RadioMenuItem*>(
						sigc::mem_fun(*this, &ColourMenu::selection),
						item));
		}

		i1 = new Gtk::MenuItem();
		append(*manage(i1));
		i1 = new Gtk::MenuItem("Smooth");
		i1->set_sensitive(false);
		append(*manage(i1));

		SmoothPalette::register_base();
		std::set<std::string> smooths = SmoothPalette::all.names();
		for (it = smooths.begin(); it != smooths.end(); it++) {
			Gtk::RadioMenuItem *item = new Gtk::RadioMenuItem(group, it->c_str());
			append(*manage(item));

			if (0==strcmp(initial.c_str(),it->c_str())) {
				item->set_active(true);
				got_init = true;
			}
			item->signal_activate().connect(
					sigc::bind<Gtk::RadioMenuItem*>(
						sigc::mem_fun(*this, &ColourMenu::selection),
						item));
		}

		if (!got_init) {
			std::cerr << "FATAL: Initial palette selection " << initial << " not found. Link error?" << std::endl;
			abort();
		}
		selection1(initial);
	}
	void selection(Gtk::RadioMenuItem *item) {
		selection1(item->get_label());
	}
	void selection1(const std::string& lbl) {
		BasePalette *sel = DiscretePalette::all.get(lbl);
		if (!sel)
			sel = SmoothPalette::all.get(lbl);
		if (sel) {
			_parent->pal=sel;
			_parent->recolour();
		}
	}
};


Menus::Menus(MainWindow& parent, std::string& init_fractal, std::string& init_colour) : main("_Main", true), plot("_Plot", true), options("_Options", true), fractal("_Fractal", true), colour("_Colour", true) {
	append(main);
	main.set_submenu(*manage(new MainMenu()));
	append(plot);
	plot.set_submenu(*manage(new PlotMenu(parent)));
	append(options);
	options.set_submenu(*manage(new OptionsMenu(parent)));
	append(fractal);
	fractal.set_submenu(*manage(new FractalMenu(parent, init_fractal)));
	append(colour);
	colour.set_submenu(*manage(new ColourMenu(parent, init_colour)));
}

} // end namespace
