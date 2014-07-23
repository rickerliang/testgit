#ifndef CUTTER_H
#define CUTTER_H

#include <functional>

#include <QFileInfo>
#include <QObject>

struct CropParameter
{
	double m_posX;
	double m_posY;
	double m_scale;
	CropParameter()
		: m_posX(0.0)
		, m_posY(0.0)
		, m_scale(0.0)
	{

	}
};

class cutter : public QObject
{
	Q_OBJECT

public:
	cutter();
	~cutter();

	void cut(QFileInfoList* l, const QString& outputDir, const QString& inputDir);
	void asyncCut(QFileInfoList* l, const QString& outputDir, const QString& inputDir);
	void emitTick(int count);
	void setRetriever(std::function<void(int index, CropParameter& out)> f);
	std::function<void(int index, CropParameter& out)> getRetriever();

signals:
	void tick(int count);

private:
	std::function<void(int index, CropParameter& out)> m_parameterRetriever;
};

#endif // CUTTER_H
