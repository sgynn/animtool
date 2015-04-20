#include <QApplication>
#include "animtool.h"

int main(int argc, char* argv[]) {
	//Initialise qt
	QApplication app(argc, argv);
	//Create test dialog
	AnimTool* window = new AnimTool();
	window->show();
	//Run!
	return app.exec();
}


