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
	// pos[-1, 1] scale[0,1]
	void getCropParameter(int index, CropParameter& out);

private slots:
	void textChanged(const QString& text);
	void buttonClicked(QAbstractButton* button);
	void inputButtonClicked(bool checked);
	void outputButtonClicked(bool checked);
	void goButtonClicked(bool checked);
	void tick(int count);

private:
	struct InputParameter;
	void previewDrawHandler(std::function<void()> supperHandler, QPaintEvent* event);
	void retrieveScaleParameter();
	void CropParameterLerp(
		const InputParameter& input, double t, CropParameter& output);

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

	struct InputParameter
	{
		double m_beginPosX;
		double m_beginPosY;
		double m_beginScale;
		double m_endPosX;
		double m_endPosY;
		double m_endScale;
		InputParameter()
			: m_beginPosX(0.0)
			, m_beginPosY(0.0)
			, m_beginScale(0.0)
			, m_endPosX(0.0)
			, m_endPosY(0.0)
			, m_endScale(0.0)
		{

		}
	};
	InputParameter m_intputParameter;
};

#endif // TIMESCAPE_IMAGE_CUTTER_H
