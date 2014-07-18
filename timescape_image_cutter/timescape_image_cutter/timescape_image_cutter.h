#ifndef TIMESCAPE_IMAGE_CUTTER_H
#define TIMESCAPE_IMAGE_CUTTER_H

#include <functional>

#include <QtGui/QMainWindow>
#include "ui_timescape_image_cutter.h"

class timescape_image_cutter : public QMainWindow
{
	Q_OBJECT

public:
	timescape_image_cutter(QWidget *parent = 0, Qt::WFlags flags = 0);
	~timescape_image_cutter();
	void setBackground(const QPixmap& p);
	void setScale(double s);
	void setCuttingPos(double x, double y);

private slots:
	void textChanged(const QString& text);

private:
	void previewDrawHandler(std::function<void()> supperHandler, QPaintEvent* event);

	Ui::timescape_image_cutterClass ui;
	double m_backgroundPicRatio;
	double m_posX;
	double m_posY;
	QPixmap m_backgroundPic;
	double m_scale;
};

#endif // TIMESCAPE_IMAGE_CUTTER_H
