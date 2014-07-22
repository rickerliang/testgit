#ifndef CUTTER_H
#define CUTTER_H

#include <QFileInfo>
#include <QObject>

class cutter : public QObject
{
	Q_OBJECT

public:
	cutter();
	~cutter();

	void cut(QFileInfoList* l, const QString& outputDir, const QString& inputDir);
	void asyncCut(QFileInfoList* l, const QString& outputDir, const QString& inputDir);
	void emitTick(int count);

signals:
	void tick(int count);

private:
	QFileInfoList m_files;
};

#endif // CUTTER_H
