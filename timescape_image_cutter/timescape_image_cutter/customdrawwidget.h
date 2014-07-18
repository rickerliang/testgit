#ifndef QCUSTOMDRAWWIDGET_H
#define QCUSTOMDRAWWIDGET_H

#include <functional>

#include <QWidget>

class CustomDrawWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CustomDrawWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
    ~CustomDrawWidget();

	void setCustomizeDrawHandler(
        std::function<
            void(std::function<void()> supperHandler, QPaintEvent* event)> handler);

protected:
    virtual void paintEvent(QPaintEvent * event) override;

private:
    void supperPaint(QPaintEvent* event);   

    std::function<
        void(std::function<void()> supperHandler, QPaintEvent* event)> m_customizeDrawHandler;
};

#endif // QCUSTOMDRAWWIDGET_H
