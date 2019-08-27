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
    void pause();
    void resume();

private:
    int m_originalValue;
    int m_targetValue;
    int m_duration;
    QTimer m_ticker;
    QTime m_stopwatch;
};

#endif // QDYNAMICBAR_H
