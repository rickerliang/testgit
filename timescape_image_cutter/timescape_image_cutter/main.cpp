#include "timescape_image_cutter.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	timescape_image_cutter w;
	w.show();
	return a.exec();
}
