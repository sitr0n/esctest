#include "qdynamicbar.h"

QDynamicBar::QDynamicBar(QWidget *parent)
    : QProgressBar(parent)
{
    setStyleSheet("QProgressBar {"
                  "border: 2px solid grey;"
                  "border-radius: 5px;"
                  "text-align: center;"
                  "background-color: #ECECEC"
                  "}"
                  "QProgressBar::chunk {"
                  "background-color: #05B8CC;"
                  //"width: 10px;"
                  //"margin: 0.5px;"
                  "}");
    m_stopwatch.start();
    connect(&m_ticker, &QTimer::timeout, this,
            [=](){
        double progress = 1.0 - (double(m_duration - m_stopwatch.elapsed()) / m_duration);
        QProgressBar::setValue( std::abs(m_targetValue - m_originalValue)*progress + m_originalValue );
        if (m_stopwatch.elapsed() >= m_duration) {
            QProgressBar::setValue(m_targetValue);
            m_ticker.stop();
        }
    });
}

void QDynamicBar::glideTo(int value, int timeSpan)
{
    if (this->value() == value) {
        return;
    }
    m_originalValue = this->value();
    m_targetValue = value;
    m_duration = timeSpan;
    auto refreshInterval = (int)m_duration / std::abs(m_targetValue - m_originalValue);
    m_ticker.start(refreshInterval);
    m_stopwatch.restart();
}

void QDynamicBar::setValue(int value)
{
    m_ticker.stop();
    QProgressBar::setValue(value);
}

void QDynamicBar::pause()
{
    m_ticker.stop();
}

void QDynamicBar::resume()
{
    m_ticker.start();
}
