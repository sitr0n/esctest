#include <QPainter>
#include <QWidget>
#include <QtGui>
#include <QTimer>
#include <QRadialGradient>
#include <QGraphicsEllipseItem>


#include "statusbitwidget.h"

StatusBitWidget::StatusBitWidget(QWidget *parent) :
    QWidget(parent)
{
    this->activeState = StatusBitWidget::INACTIVE_STATE;

}

StatusBitWidget::~StatusBitWidget()
{

}

void StatusBitWidget::changeEvent(QEvent *event)
{
    if(event->EnabledChange){
        this->update();
    }
}

void StatusBitWidget::paintEvent(QPaintEvent*)
{

    QPainter painter(this);


    int h = height();
    int w = width();
    float cx=w/2.0;
    float cy=w/2.0;
    QRadialGradient radialGrad(QPointF(cx, cy), cx);
    //radialGrad.setColorAt(4.0/7.0, QColor(180, 180, 180));
    //radialGrad.setColorAt(5.0/7.0, QColor(229, 228, 226));
    //radialGrad.setColorAt(6.0/7.0, QColor(145, 146, 146));
    //radialGrad.setColorAt(4.0/7.0, QColor(229, 228, 226));
    //radialGrad.setColorAt(4.0/, QWidget::palette().color(QWidget::backgroundRole()));

    //QColor bcColor = QWidget().palette().background().color();
    radialGrad.setColorAt(1.0, QColor(0,0,0,0));
    //radialGrad.setColorAt(1.0, QWidget::palette().color(QWidget::backgroundRole()));

    StatusState actualState=this->activeState;
    if(!this->isEnabled()){
        this->activeState=StatusBitWidget::INACTIVE_STATE;
    }

    switch(this->activeState)
    {
    case StatusBitWidget::INACTIVE_STATE:
        //radialGrad.setColorAt(0,       QColor(237, 237, 237));
        //radialGrad.setColorAt(3.0/7.0, QColor(237, 237, 237));
        //radialGrad.setColorAt(4.0/7.0, QColor(232, 232, 232));
        //radialGrad.setColorAt(0,       QColor(250, 250, 250));
        //radialGrad.setColorAt(2.0/7.0, QColor(233, 233, 233));
        //radialGrad.setColorAt(3.0/7.0, QColor(213, 216, 220));
        //radialGrad.setColorAt(0,       QColor(250, 250, 255));
        //radialGrad.setColorAt(0.0/7.0, QColor(233, 233, 255));
        radialGrad.setColorAt(0.0/7.0, QColor(213, 216, 255));
        radialGrad.setColorAt(3.0/7.0, QColor(190, 190, 190));
        break;

    case StatusBitWidget::ACTIVE_STATE:
        radialGrad.setColorAt(0,       QColor(0, 206,  0));
        radialGrad.setColorAt(2.0/7.0, QColor(0, 157,  0));
        radialGrad.setColorAt(3.0/7.0, QColor(0, 98,   0));
        break;

    case StatusBitWidget::ERROR_STATE:
        radialGrad.setColorAt(0,       QColor(243, 109, 116));
        radialGrad.setColorAt(2.0/7.0, QColor(237,  28,  36));
        radialGrad.setColorAt(3.0/7.0, QColor(120,  10,  15));
        break;

    case StatusBitWidget::WARNING_STATE:
        radialGrad.setColorAt(0,       QColor(255, 242,   0));
        radialGrad.setColorAt(2.0/7.0, QColor(255, 201,  14));
        radialGrad.setColorAt(3.0/7.0, QColor(179, 125,   0));
        break;

    case StatusBitWidget::ACTIVE_BLUE:
        radialGrad.setColorAt(0,       QColor(40, 162, 209));
        radialGrad.setColorAt(2.0/7.0, QColor(15, 131, 199));
        radialGrad.setColorAt(3.0/7.0, QColor(30, 36, 87));
        break;
    }
    this->activeState=actualState;

    QRect rect_radial(0,0,w,h);

    painter.fillRect(rect_radial, radialGrad);
    //QRect ellipse_rect(0,0,w-1,h-1);
    //painter.drawEllipse(ellipse_rect);
}



void StatusBitWidget::setActiveState(StatusState state)
{
    this->activeState = state;
    this->update();
}

void StatusBitWidget::setActive()
{
    this->activeState = ACTIVE_STATE;
    this->update();
}

void StatusBitWidget::setInactive()
{
    this->activeState = INACTIVE_STATE;
    this->update();
}

void StatusBitWidget::setGreen()
{
    this->activeState = ACTIVE_STATE;
    this->update();
}

void StatusBitWidget::setBlue()
{
    this->activeState = ACTIVE_BLUE;
    this->update();
}

void StatusBitWidget::setYellow()
{
    this->activeState = WARNING_STATE;
    this->update();
}

void StatusBitWidget::setRed()
{
    this->activeState = ERROR_STATE;
    this->update();
}

void StatusBitWidget::setApproved(bool bApproved){
    if(bApproved){
        this->activeState=ACTIVE_STATE;
    } else {
        this->activeState=ERROR_STATE;
    }
    this->update();
}

StatusBitWidget::StatusState StatusBitWidget::GetState(){
    return activeState;
}
