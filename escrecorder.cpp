#include "escrecorder.h"

EscRecorder::EscRecorder(WingSlot::SlotList units, QObject *parent)
    : QObject(parent)
    , m_units(units)
{
    m_log.setTitle("eB_WST_002_log");
    m_log.addColumn("InputCurrent");
    m_log.addColumn("OutputCurrent");
    m_log.addColumn("SlotLoss");
    m_log.addColumn("Temperature");
    m_log.addColumn("Charging"); // -Age ? Period? Succession? Duration?
    m_log.addColumn("Pairing"); // -Age ?
    m_log.addColumn("WingSerial");
    m_log.addColumn("SlotLQI");
    m_log.addColumn("WingLQI");
    m_log.addColumn("WingLoss");
    m_log.addColumn("WingTemperature");

    connect(&m_ticker, &QTimer::timeout, this,
            [=](){
        for (const WingSlot& unit : m_units) {
            m_log.setValue("SerialNo", unit.id());
            m_log.setValue("InputCurrent", unit.stats().iSupply);
            m_log.setValue("OutputCurrent", unit.stats().iSupplyWing);
            m_log.setValue("SlotLoss", unit.stats().loss);
            m_log.setValue("Temperature", unit.stats().temperature);
            m_log.setValue("Charging", unit.isCharging());
            m_log.setValue("Pairing", unit.isPaired());
            if (unit.isPaired()) {
                m_log.setValue("WingSerial", unit.stats().wing.serial);
                m_log.setValue("SlotLQI", unit.stats().LQI);
                m_log.setValue("WingLQI", unit.stats().wing.LQI);
                m_log.setValue("WingLoss", unit.stats().wing.loss);
                m_log.setValue("WingTemperature", unit.stats().wing.temperature);
            }
            m_log.save();
        }
    });
}

void EscRecorder::setPath(const QString &path)
{
    m_log.setPath(path);
}

void EscRecorder::start(int interval)
{
    // WingComInterval is now constant, making unit samples per period irrelevant
//    for (WingSlot& unit : m_units) {
//        unit.setSampling((int)(interval/SAMPLES_PER_PERIOD));
//    }
    m_ticker.start(interval);
}

void EscRecorder::stop()
{
    m_ticker.stop();
}
