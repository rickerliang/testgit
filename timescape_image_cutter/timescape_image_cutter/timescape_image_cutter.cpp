#include "timescape_image_cutter.h"

#include <QPainter>

timescape_image_cutter::timescape_image_cutter(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags | Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog)
	, m_backgroundPicRatio(16 / 9)
	, m_posX(0)
	, m_posY(0)
	, m_backgroundPic()
	, m_scale(1)
{
	ui.setupUi(this);
	ui.m_previewWidget->setCustomizeDrawHandler(
		std::bind(&timescape_image_cutter::previewDrawHandler, this,
		std::placeholders::_1, std::placeholders::_2));

	m_posX = ui.m_beginX->text().toDouble();
	m_posY = ui.m_beginY->text().toDouble();
	m_scale = ui.m_beginScale->text().toDouble();

	setBackground(QPixmap("c:\\psb.jpg"));

	connect(ui.m_beginScale, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
	connect(ui.m_beginX, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
	connect(ui.m_beginY, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
}

timescape_image_cutter::~timescape_image_cutter()
{

}

void timescape_image_cutter::previewDrawHandler(
	std::function<void()> supperHandler, QPaintEvent* event)
{
	QPainter p(ui.m_previewWidget);
	int renderTargetWidth = ui.m_previewWidget->width();
	int renderTargetHeight = ui.m_previewWidget->height();
	int picWidth = ui.m_previewWidget->height() * m_backgroundPicRatio;
	int picHeight = ui.m_previewWidget->width() / m_backgroundPicRatio;

	if (renderTargetWidth / m_backgroundPicRatio > renderTargetHeight)
 	{
		p.drawPixmap(
			(ui.m_previewWidget->width() - picWidth) / 2, 0, picWidth,
			ui.m_previewWidget->height(), m_backgroundPic);
	}
 	else
 	{		
		p.drawPixmap(
			0, (ui.m_previewWidget->height() - picHeight) / 2, 
			ui.m_previewWidget->width(), picHeight, m_backgroundPic);
 	}

	QPoint center(
		(renderTargetWidth / 2) * (1 + m_posX), 
		(renderTargetHeight / 2) * (1 + m_posY));
	int destWidth = picWidth * m_scale;
	int destHeight = picHeight * m_scale;
	p.setPen(QColor(Qt::black));
	p.drawRect(center.x() - destWidth / 2, center.y() - destHeight / 2, destWidth, destHeight);
}

void timescape_image_cutter::setBackground(const QPixmap& p)
{
	m_backgroundPic = p;
	m_backgroundPicRatio = 
		double(m_backgroundPic.width()) / double(m_backgroundPic.height());
}

void timescape_image_cutter::setScale(double s)
{
	m_scale = s;
}

void timescape_image_cutter::setCuttingPos(double x, double y)
{
	m_posX = x;
	m_posY = y;
}

void timescape_image_cutter::textChanged(const QString& text)
{
	bool ret = false;
	text.toDouble(&ret);
	if (!ret)
	{
		return ;
	}

	if (ui.m_radioButtonBegin->isChecked())
	{
		m_posX = ui.m_beginX->text().toDouble();
		m_posY = ui.m_beginY->text().toDouble();
		m_scale = ui.m_beginScale->text().toDouble();
	}
	else
	{
		m_posX = ui.m_endX->text().toDouble();
		m_posY = ui.m_endY->text().toDouble();
		m_scale = ui.m_endScale->text().toDouble();
	}	
	update();
}
