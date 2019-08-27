#ifndef ESCMONITOR_H
#define ESCMONITOR_H

#include <QWidget>
#include "wingslot.h"
#include "palm.h"
#include <QTimer>
#include <QTime>
#include <QPushButton>
#include <QLabel>
#include "widgets/statusbitwidget.h"
#include "GraphWidget/graphwidget.h"

class EscMonitor : public QWidget
{
    Q_OBJECT
public:
    explicit EscMonitor(QWidget *parent = nullptr);
    void display(WingSlot *unit, bool editable = true);

signals:
    void warning(const QString &message);
    void error(const QString &message);

protected slots:
    void displayData(const WingSlot::Stats &stats);

private:
    WingSlot *m_unit;
    QTimer m_poller;
    QTime m_stopwatch;
    int m_interval;
    int m_logInterval;
    bool m_charging;
    bool m_isMutable;

    QWidget m_slotPanel;
    QLabel m_slotSerial;
    QLabel m_slotTemperature;
    QLabel m_slotLQI;
    QLabel m_slotLoss;
    QLabel m_slotCurrent;
    QPushButton *m_chargingButton;
    StatusBitWidget *m_chargingLED;

    QWidget m_wingPanel;
    QLabel m_wingSerial;
    QLabel m_wingTemperature;
    QLabel m_wingLQI;
    QLabel m_wingLoss;
    QLabel m_wingCurrent;
    QPushButton *m_pairingButton;
    StatusBitWidget *m_pairingLED;

    QWidget m_dataPanel;
    GraphWidget *m_dataGraph;

    const int DATA_DECIMALS = 2;
    const int LED_SIZE = 30;
};

#endif // ESCMONITOR_H



