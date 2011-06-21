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
		ZoomIn.signal_activate().connect(sigc::mem_fun(this, &PlotMenu::do_zoom_in));

		append(ZoomOut);
		ZoomOut.add_accelerator("activate", ag, GDK_minus, Gdk::ModifierType::SHIFT_MASK, Gtk::ACCEL_VISIBLE);
		ZoomOut.signal_activate().connect(sigc::mem_fun(this, &PlotMenu::do_zoom_out));

		append(Sepa2);
		append(Stop);
		Stop.add_accelerator("activate", ag, GDK_period, Gdk::ModifierType::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
		Stop.signal_activate().connect(sigc::mem_fun(this, &PlotMenu::do_stop));

		append(Redraw);
		Redraw.set_label("Redraw");
		Redraw.add_accelerator("activate", ag, GDK_R, Gdk::ModifierType::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
		Redraw.signal_activate().connect(sigc::mem_fun(this, &PlotMenu::do_redraw));

		append(More);
		More.set_label("More iterations");
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
	void do_zoom_in() {
		MainWindow *mw = find_main(this);
		mw->do_zoom(MainWindow::Zoom::ZOOM_IN);
	}
	void do_zoom_out() {
		MainWindow *mw = find_main(this);
		mw->do_zoom(MainWindow::Zoom::ZOOM_OUT);
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

Menus::Menus(MainWindow& parent) : main("Main"), plot("Plot"), options("Options"), fractal("Fractal"), colour("Colour") {
	    append(main);
	    main.set_submenu(*manage(new MainMenu()));
	    append(plot);
	    plot.set_submenu(*manage(new PlotMenu(parent)));
	    append(options);
	    append(fractal);
	    append(colour);
#if 0 //XXX UISLOG
	    options.set_submenu(*manage(new OptionsMenu()));
	    // refactor: setup_fractal_menu(&gtk_ctx, menubar, "Mandelbrot");
	    fractal.set_submenu(*manage(new FractalMenu()));
	    // refactor: setup_colour_menu(&gtk_ctx, menubar, "Linear rainbow");
	    colour.set_submenu(*manage(new ColorMenu()));

	    // TODO Kill off uixml
	    // TODO Ensure our custom accelerators work
#endif
	  }
}

#if 0
<ui>
	<menubar>
		<menu name="OptionsMenu" action="OptionsMenuAction">
			<menuitem name="DrawHUD" action="DrawHUDAction" />
			<menuitem name="AntiAlias" action="AntiAliasAction" />
		</menu>
	</menubar>
</ui>
#endif
