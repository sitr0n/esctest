/*******************************************************************************
*      $URL: https://dr-visualsvn/svn/NO_seatex_rttb/trunk/control/RTTB Control (GUI)/modulatorswitchcontrolwidget.h $
* $Revision: 1 $
*   $Author: sba@DATA-RESPONS $
*     $Date: 23.04.2014 $
*
*   Project: Kongsberg Seatex
*    Author: SBA (DR)
*  Compiler: MinGW/GCC
*    Target: PC/Laptop with Qt support
***********************************************************************
*
*  Description:
*
*  Custom widget imitating a switch component to select modulator output destination
*
*******************************************************************************/
#ifndef STATUSBITWIDGET_H
#define STATUSBITWIDGET_H

#include <QWidget>
#include <QString>

class StatusBitWidget :
        public QWidget
{
    Q_OBJECT

public:
    StatusBitWidget(QWidget *parent = 0);
    ~StatusBitWidget();
    void changeEvent(QEvent *event);

    enum StatusState {
        INACTIVE_STATE,
        ACTIVE_STATE,
        ERROR_STATE,
        WARNING_STATE,
        ACTIVE_BLUE
    };

    StatusState GetState();
    void setGreen();
    void setBlue();
    void setYellow();
    void setRed();
    void setApproved(bool bApproved);

Q_SIGNALS:
    //void clicked();

public Q_SLOTS:
    void setActiveState(StatusState state);
    void setActive();
    void setInactive();

protected:
    void paintEvent(QPaintEvent*);
    //void mousePressEvent(QMouseEvent*);

private Q_SLOTS:
    //void autoOffTimeout();

private:
    StatusState activeState;
};

#endif // STATUSBITWIDGET_H
