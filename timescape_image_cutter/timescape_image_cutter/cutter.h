#ifndef CUTTER_H
#define CUTTER_H

#include <QFileInfo>
#include <QObject>

class cutter : public QObject
{
	Q_OBJECT

public:
	cutter(QObject *parent);
	~cutter();

	void cut(QFileInfoList l);

signals:
	void tick();

private:
	
};

#endif // CUTTER_H
