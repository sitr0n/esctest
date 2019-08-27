#ifndef ESCRECORDER_H
#define ESCRECORDER_H

#include <QObject>
#include <QTimer>
#include "wingslot.h"
#include "palm.h"


class EscRecorder : public QObject
{
    Q_OBJECT
public:
    explicit EscRecorder(WingSlot::SlotList units, QObject *parent = nullptr);
    void setPath(const QString &path);
    void start(int interval);
    void stop();

private:
    WingSlot::SlotList m_units;
    PALM m_log;
    QTimer m_ticker;

    const int SAMPLES_PER_PERIOD = 1;
};

#endif // ESCRECORDER_H
