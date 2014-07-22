#include "cutter.h"

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
		const QFileInfoList& l = m_l;
		for(size_t i = r.begin(); i != r.end( ); ++i)
		{
			QImage p(l[i].absoluteFilePath());
			p.save(m_outputDir.absoluteFilePath(l[i].fileName()));
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
