#ifndef TIMESCAPE_IMAGE_CUTTER_H
#define TIMESCAPE_IMAGE_CUTTER_H

#include <functional>
#include <memory>

#include <QFileInfo>
#include <QtGui/QMainWindow>
#include "ui_timescape_image_cutter.h"
#include "cutter.h"

class timescape_image_cutter : public QMainWindow
{
	Q_OBJECT

public:
	timescape_image_cutter(QWidget *parent = 0, Qt::WFlags flags = 0);
	~timescape_image_cutter();
	void setBackground(const QPixmap& p);

private slots:
	void textChanged(const QString& text);
	void buttonClicked(QAbstractButton* button);
	void inputButtonClicked(bool checked);
	void outputButtonClicked(bool checked);
	void goButtonClicked(bool checked);
	void tick(int count);

private:
	void previewDrawHandler(std::function<void()> supperHandler, QPaintEvent* event);
	void retrieveScaleParameter();

	Ui::timescape_image_cutterClass ui;
	double m_backgroundPicRatio;
	double m_posX;
	double m_posY;
	QPixmap m_backgroundPic;
	double m_scale;
	QFileInfoList m_files;
	cutter m_cutter;
	QString m_outputDir;
	QString m_inputDir;
};

#endif // TIMESCAPE_IMAGE_CUTTER_H
