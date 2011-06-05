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
#include <gtkmm/main.h>
#include <gtkmm/menu.h>
#include <gtkmm/stock.h>
#include <gtkmm/aboutdialog.h>
#include <gdkmm/pixbuf.h>

#include "config.h"
#include "gtkmain.h"
#include "logo.h" // in libbrot2

namespace menus {

class MainMenu : public Gtk::Menu {
public:
	MainMenu() : aboutI(Gtk::Stock::ABOUT), saveI(Gtk::Stock::SAVE), sepa(), quitI(Gtk::Stock::QUIT) {
		append(aboutI);
		aboutI.signal_activate().connect(sigc::ptr_fun(do_about));
		append(saveI); // XXX TODO: IMPLEMENT ME, need a sigc with an object?
		// saveI.signal_activate().connect(sigc::mem_fun(*this, do_save));
		append(sepa);
		append(quitI);
		quitI.signal_activate().connect(sigc::ptr_fun(do_quit));
	}
	Gtk::ImageMenuItem aboutI, saveI, sepa, quitI;

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
};

Menus::Menus() : main("Main"), plot("Plot"), options("Options"), fractal("Fractal"), colour("Colour") {
	    append(main);
	    main.set_submenu(*manage(new MainMenu()));
#if 0
	    append(plot);
	    plot.set_submenu(*manage(new PlotMenu()));
	    append(options);
	    options.set_submenu(*manage(new OptionsMenu()));
	    append(fractal);
	    // refactor: setup_fractal_menu(&gtk_ctx, menubar, "Mandelbrot");
	    fractal.set_submenu(*manage(new FractalMenu()));
	    append(colour);
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
		<menu name="MainMenu" action="MainMenuAction">
			<menuitem name="About" action="AboutAction" />
			<menuitem name="SaveImage" action="SaveImageAction" />
			<separator/>
			<menuitem name="Quit" action="QuitAction" />
		</menu>
		<menu name="PlotMenu" action="PlotMenuAction">
			<menuitem name="Undo" action="UndoAction" />
			<menuitem name="Parameters" action="ParametersAction" />
			<separator/>
			<menuitem name="ZoomIn" action="ZoomInAction" />
			<menuitem name="ZoomOut" action="ZoomOutAction" />
			<separator/>
			<menuitem name="Stop" action="StopAction" />
			<menuitem name="Redraw" action="RedrawAction" />
			<menuitem name="MoreIterations" action="MoreIterationsAction" />
			<!--
				 <menuitem name="x" action="xAction" />
				 -->
		</menu>
		<menu name="OptionsMenu" action="OptionsMenuAction">
			<menuitem name="DrawHUD" action="DrawHUDAction" />
			<menuitem name="AntiAlias" action="AntiAliasAction" />
		</menu>
	</menubar>
</ui>
#endif
