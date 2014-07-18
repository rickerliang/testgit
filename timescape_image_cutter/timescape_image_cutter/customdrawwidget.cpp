#include "customdrawwidget.h"

CustomDrawWidget::CustomDrawWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_customizeDrawHandler()
{

}

CustomDrawWidget::~CustomDrawWidget()
{

}

void CustomDrawWidget::paintEvent(QPaintEvent * event)
{
    std::function<void()> supperHandler = 
        [this, event]()
    {
        supperPaint(event);
    };

    if (m_customizeDrawHandler)
    {
        m_customizeDrawHandler(supperHandler, event);
    }
    else
    {
        supperHandler();
    }
}

void CustomDrawWidget::supperPaint(QPaintEvent* event)
{
    __super::paintEvent(event);
}

void CustomDrawWidget::setCustomizeDrawHandler(
    std::function<
        void(std::function<void()> supperHandler, QPaintEvent* event)> handler)
{
    m_customizeDrawHandler = handler;
}

