#include "timescape_image_cutter.h"
#include <QtGui/QApplication>

#include <tbb/task_scheduler_init.h>

int main(int argc, char *argv[])
{
	tbb::task_scheduler_init init(tbb::task_scheduler_init::deferred);
	init.initialize(/*tbb::task_scheduler_init::automatic*/1);
	QApplication a(argc, argv);
	timescape_image_cutter w;
	w.show();
	a.exec();
	init.terminate();
	return 0;
}
