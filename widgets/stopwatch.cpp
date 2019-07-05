#include "stopwatch.h"
#include <QRegExp>
#include <QDebug>
#include <QStringList>
#define DISPLAY_DIGITS 8
#define HZ_1 1000
#define HZ_2 500
#define MS_PER_SEC 1000
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

Stopwatch::Stopwatch(QWidget *parent) :
    QLCDNumber(parent),
    m_duration_seconds(0),
    m_emit_signal(false),
    m_running(false)
{
    this->setDigitCount(DISPLAY_DIGITS);
    this->display(QString("00:00:00"));
    this->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    this->setMinimumSize(QSize(100, 50));
    m_ticker.setInterval(HZ_2);
    connect(&m_ticker, SIGNAL(timeout()), this, SLOT(update()));
}

void Stopwatch::set(int duration_seconds)
{
    m_duration_seconds = 0;
    m_emit_signal = false;
    m_running = false;
    if (0 < duration_seconds) {
        m_duration_seconds = duration_seconds;
        m_emit_signal = true;
    }
    update();
}

void Stopwatch::set(QString timeout)
{
    QRegExp format("\\d\\d:\\d\\d:\\d\\d");
    if (format.exactMatch(timeout)) {
        QStringList tolist = timeout.split(QChar(':'));
        int duration_seconds = 0;
        duration_seconds += QString(tolist[0]).toInt() * SEC_PER_HOUR;
        duration_seconds += QString(tolist[1]).toInt() * SEC_PER_MIN;
        duration_seconds += QString(tolist[2]).toInt();
        set(duration_seconds);
    }
}

void Stopwatch::start()
{
    m_timer.start();
    m_ticker.start();
    m_running = true;
}

void Stopwatch::start(int hours, int minutes, int seconds)
{
    int duration_seconds = 0;
    duration_seconds += hours * SEC_PER_HOUR;
    duration_seconds += minutes * SEC_PER_MIN;
    duration_seconds += seconds;

    set(duration_seconds);
    start();
}

void Stopwatch::stop()
{
    m_ticker.stop();
    m_duration_seconds = 0;
    m_emit_signal = false;
    m_running = false;
    this->display(to_string(QTime(0, 0, 0)));
}

bool Stopwatch::isRunning() const
{
    return m_running;
}

void Stopwatch::update()
{
    int seconds_elapsed = (int) m_timer.elapsed()/MS_PER_SEC;
    int remaining_time = m_duration_seconds - seconds_elapsed;
    if (m_emit_signal && remaining_time <= 0) {
        emit timeout();
        m_emit_signal = false;
        m_running = false;
    }

    if (remaining_time < 0) { // Absolute value
        remaining_time *= -1;
    }
    int timer_h = ((int) remaining_time/SEC_PER_HOUR) % 24;
    int timer_m = ((int) remaining_time/SEC_PER_MIN) % 60;
    int timer_s = remaining_time % 60;
    QTime timer(timer_h, timer_m, timer_s);
    this->display(to_string(timer));
}

QString Stopwatch::to_string(QTime time)
{
    QString display_string;
    int hours = time.hour();
    if (hours < 10) {
        display_string += "0";
    }
    display_string += QString::number(hours) + ":";
    int minutes = time.minute();
    if (minutes < 10) {
        display_string += "0";
    }
    display_string += QString::number(minutes) + ":";
    int seconds = time.second();
    if (seconds < 10) {
        display_string += "0";
    }
    display_string += QString::number(seconds);

    return display_string;
}
