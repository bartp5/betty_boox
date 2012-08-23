/*
 * betty.cpp
 *
 * Main routine for the betty terminal emulator for the boox m92
 *
 *
 * 23.08.2012 Bart
 */
#include <QApplication>
#include "/opt/onyx/arm/include/onyx/sys/sys.h"
#include "/opt/onyx/arm/include/onyx/ui/password_dialog.h"
#include "/opt/onyx/arm/include/onyx/screen/screen_update_watcher.h"

#include "mainwindow.h"



int main(int argc, char *argv[])
{

	QApplication app(argc, argv);
	app.setApplicationName("Betty Boox Terminal Emulator");
	ui::BooxTerminal term_wnd(0);
	onyx::screen::watcher().addWatcher(&term_wnd);        //watches for refresh triggering events
	//input_wnd.show();
	term_wnd.showMaximized();
	sys::SysStatus::instance().setSystemBusy(false);
	term_wnd.exec();
	return 0;
}
