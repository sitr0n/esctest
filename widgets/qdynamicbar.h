#ifndef QDYNAMICBAR_H
#define QDYNAMICBAR_H

#include <QProgressBar>
#include <QTimer>
#include <QTime>

class QDynamicBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit QDynamicBar(QWidget *parent = 0);
    ~QDynamicBar() = default;
    void glideTo(int value, int timeSpan);
    void setValue(int value);

private:
    int originalValue;
    int targetValue;
    int duration;
    QTimer ticker;
    QTime stopwatch;
};

#endif // QDYNAMICBAR_H
