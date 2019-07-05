#include "qdynamicbar.h"

QDynamicBar::QDynamicBar(QWidget *parent)
    : QProgressBar(parent)
{
    stopwatch.start();
    connect(&ticker, &QTimer::timeout, this,
            [=](){
        double progress = 1.0 - (double(duration - stopwatch.elapsed()) / duration);
        QProgressBar::setValue( std::abs(targetValue - originalValue)*progress + originalValue );
        if (stopwatch.elapsed() >= duration) {
            QProgressBar::setValue(targetValue);
            ticker.stop();
        }
    });
}

void QDynamicBar::glideTo(int value, int timeSpan)
{
    originalValue = this->value();
    targetValue = value;
    duration = timeSpan;
    auto refreshInterval = (int)duration / std::abs(targetValue - originalValue);
    ticker.start(2*refreshInterval);
    stopwatch.restart();
}

void QDynamicBar::setValue(int value)
{
    ticker.stop();
    QProgressBar::setValue(value);
}
