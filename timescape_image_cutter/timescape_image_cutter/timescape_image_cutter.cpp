#include "timescape_image_cutter.h"

#include <cassert>

#include <QFileDialog>
#include <QPainter>

#include "cutter.h"

timescape_image_cutter::timescape_image_cutter(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags | Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog)
	, m_backgroundPicRatio(16 / 9)
	, m_posX(0)
	, m_posY(0)
	, m_backgroundPic()
	, m_scale(1)
	, m_files()
	, m_cutter()
	, m_outputDir("\\output")
	, m_inputDir()
{
	ui.setupUi(this);
	ui.m_previewWidget->setCustomizeDrawHandler(
		std::bind(&timescape_image_cutter::previewDrawHandler, this,
		std::placeholders::_1, std::placeholders::_2));

	m_posX = ui.m_beginX->text().toDouble();
	m_posY = ui.m_beginY->text().toDouble();
	m_scale = ui.m_beginScale->text().toDouble();

	ui.m_outputLabel->setText(m_outputDir);

	connect(ui.m_beginScale, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
	connect(ui.m_beginX, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
	connect(ui.m_beginY, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
	connect(ui.m_endX, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
	connect(ui.m_endY, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
	connect(ui.m_endScale, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
	connect(ui.buttonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
	connect(ui.m_inputButton, SIGNAL(clicked(bool)), this, SLOT(inputButtonClicked(bool)));
	connect(ui.m_buttonGo, SIGNAL(clicked(bool)), this, SLOT(goButtonClicked(bool)));
	connect(&m_cutter, SIGNAL(tick(int)), this, SLOT(tick(int)));
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
		picHeight = ui.m_previewWidget->height();
		p.drawPixmap(
			(ui.m_previewWidget->width() - picWidth) / 2, 0, picWidth,
			picHeight, m_backgroundPic);
	}
 	else
 	{
		picWidth = ui.m_previewWidget->width();
		p.drawPixmap(
			0, (ui.m_previewWidget->height() - picHeight) / 2, 
			picWidth, picHeight, m_backgroundPic);
 	}

	QPoint center(
		(renderTargetWidth / 2) + (picWidth / 2) * m_posX, 
		(renderTargetHeight / 2) + (picHeight / 2) * m_posY);
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

void timescape_image_cutter::textChanged(const QString& text)
{
	bool ret = false;
	text.toDouble(&ret);
	if (!ret)
	{
		return ;
	}
	retrieveScaleParameter();
	update();
}

void timescape_image_cutter::buttonClicked(QAbstractButton* button)
{
	retrieveScaleParameter();
	update();
}

void timescape_image_cutter::retrieveScaleParameter()
{
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
}

void timescape_image_cutter::inputButtonClicked(bool checked)
{
	QString d = QFileDialog::getExistingDirectory(
		this, tr("Open Directory"), "", 
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	
	if (d.isEmpty())
	{
		return ;
	}
	m_inputDir = d;
	ui.m_inputLabel->setText(m_inputDir);
	m_outputDir = d + "\\output";
	ui.m_outputLabel->setText(m_outputDir);
	QDir dir(m_inputDir);
	QStringList l;
	l.append("*.jpg");
	l.append("*.jpeg");
	m_files = dir.entryInfoList(l, QDir::Files, QDir::Name);
	if (m_files.size() > 0)
	{
		setBackground(QPixmap((m_files)[0].absoluteFilePath()));
	}
}

void timescape_image_cutter::goButtonClicked(bool checked)
{
	ui.m_progressBar->setValue(0);
	ui.m_progressBar->setMaximum(m_files.size());
	m_cutter.asyncCut(&m_files, m_outputDir, m_inputDir);
}

void timescape_image_cutter::tick(int count)
{
	ui.m_progressBar->setValue(ui.m_progressBar->value() + count);
}

void timescape_image_cutter::outputButtonClicked(bool checked)
{
	QString d = QFileDialog::getExistingDirectory(
		this, tr("Open Directory"), "", 
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (d.isEmpty())
	{
		return ;
	}
	m_outputDir = d;
	ui.m_outputLabel->setText(m_outputDir);
}
