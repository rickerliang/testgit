#include "cutter.h"

#include <QDebug>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

namespace
{
class CutterFunctor
{
public:
	CutterFunctor(QFileInfoList& l)
		: m_l(l)
	{

	}

	void operator()(const tbb::blocked_range<size_t>& r) const
	{
		QFileInfoList& l = m_l;
		for(size_t i = r.begin(); i != r.end( ); ++i)
		{
			qDebug() << m_l[i].absoluteFilePath();
		}
	}

private:
	QFileInfoList& m_l;
};
}

cutter::cutter(QObject *parent)
	: QObject(parent)
{

}

cutter::~cutter()
{

}

void cutter::cut(QFileInfoList l)
{
	tbb::parallel_for(
		tbb::blocked_range<size_t>(0, l.size()), CutterFunctor(l), 
		tbb::auto_partitioner());
}
