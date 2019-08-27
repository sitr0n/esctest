#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <QLCDNumber>
#include <QTimer>
#include <QTime>
#include <iostream>

class Stopwatch : public QLCDNumber
{
    Q_OBJECT
public:
    explicit Stopwatch(QWidget *parent = 0);
    void set(int duration_seconds);
    void set(QString timeout);
    void start();
    void start(int hours, int minutes, int seconds);
    void stop();

    bool isRunning() const;

signals:
    void timeout();

private slots:
    void update();

private:
    QTimer m_ticker;
    QTime m_timer;
    int m_duration_seconds;
    bool m_emit_signal;
    bool m_running;

    QString to_string(QTime time);

};

#endif // STOPWATCH_H
