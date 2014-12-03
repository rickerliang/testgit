#include "cutter.h"

#include <cassert>
#include <functional>

#include <QDebug>
#include <QDir>
#include <QImage>
#include <QtConcurrentRun>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

namespace
{
class CutterFunctor
{
public:
	CutterFunctor(QFileInfoList& l, cutter& c, const QString& outputDir)
		: m_l(l)
		, m_cutter(c)
		, m_outputDir(outputDir)
	{

	}

	void operator()(const tbb::blocked_range<size_t>& r) const
	{
		qDebug() << r.size();
		const QFileInfoList& l = m_l;
		QImage cropped;
		for(size_t i = r.begin(); i != r.end( ); ++i)
		{
			if (!m_cutter.getRetriever())
			{
				assert(false && "retriever not set");
				continue;
			}
			CropParameter crop;
			m_cutter.getRetriever()(i, crop);
			qDebug() << crop.m_scale << "-" << crop.m_posX << "-" << crop.m_posY;
			QImage p(l[i].absoluteFilePath());

			QSize s = p.size();
			double w = s.width() * crop.m_scale;
			double h = s.height() * crop.m_scale;
			QRect r(0, 0, w, h);
			QPoint center(
				s.width() * ((crop.m_posX + 1.0) / 2.0), 
				s.height() * ((crop.m_posY + 1.0) / 2.0));
			r.moveTo(center.x() - w / 2, center.y() - h / 2);
			cropped = p.copy(r);
			assert(!cropped.isNull());
			QString outputFileName = m_outputDir.absoluteFilePath(l[i].fileName());
			cropped.save(outputFileName);
		}
		m_cutter.emitTick(r.size());
	}

private:
	const QFileInfoList& m_l;
	cutter& m_cutter;
	QDir m_outputDir;
};
}

cutter::cutter()
	: QObject(nullptr)
	, m_parameterRetriever()
{

}

cutter::~cutter()
{

}

void cutter::asyncCut(QFileInfoList* l, const QString& outputDir, const QString& inputDir)
{
	QtConcurrent::run(std::bind(&cutter::cut, this, l, outputDir, inputDir));
}

void cutter::emitTick(int count)
{
	emit tick(count);
}

void cutter::cut(QFileInfoList* l, const QString& outputDir, const QString& intputDir)
{
	QDir d("");
	d.mkpath(outputDir);
	tbb::parallel_for(
		tbb::blocked_range<size_t>(0, l->size()), CutterFunctor(*l, *this, outputDir), 
		tbb::auto_partitioner());
}

void cutter::setRetriever(std::function<void(int index, CropParameter& out)> f)
{
	m_parameterRetriever = f;
}

std::function<void(int index, CropParameter& out)> cutter::getRetriever()
{
	return m_parameterRetriever;
}
