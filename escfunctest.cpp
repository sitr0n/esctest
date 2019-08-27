#include "escfunctest.h"
#include <QDebug>

EscFuncTest::Limits EscFuncTest::s_limit;
EscFuncTest::Durations EscFuncTest::s_duration;
const QString EscFuncTest::VERSION = "2.00.04";


EscFuncTest::EscFuncTest(WingSlot &unit, QObject *parent)
    : QObject(parent)
    , m_unit(unit)
    , m_state(State::NONE)
    , m_OK(true)
{
    m_log.setTitle("Activity_Test_118_eBird_Wing");
    m_log.setPath("~/PALM/log");

    m_log.addColumn("TestVersion");
    m_log.addColumn("Approved");
    m_log.addColumn("PairingElapsed");
    m_log.addColumn("WingSerial");
    m_log.addColumn("MeasuringDuration_noCharge");
    m_log.addColumn("iSupply_noCharge");
    m_log.addColumn("iSupplyWing_noCharge");
    m_log.addColumn("MeasurigDuration_charge");
    m_log.addColumn("iSupply_charge");
    m_log.addColumn("iSupplyWing_charge");
    m_log.addColumn("MeasuringDuration_noLoad"); // To be implemented
    m_log.addColumn("iSupply_noLoad"); // To be implemented
    m_log.addColumn("iSupplyWing_noLoad"); // To be implemented
    m_log.addColumn("WingBatCapacity");
    m_log.addColumn("WingBatCurrent");
    m_log.addColumn("WingBatVolt");
    m_log.addColumn("WingHumidity");
    m_log.addColumn("WingLoss");
    m_log.addColumn("WingLqi");
    m_log.addColumn("WingTemperature");
    m_log.addColumn("SlotLoss");
    m_log.addColumn("SlotLqi");
    m_log.addColumn("SlotTemperature");

    m_log.setValue("Product", QString("eB-WCB_001"));
    m_log.setValue("SerialNo", m_unit.id());
    m_log.setValue("ProductFamily", QString("eBird"));
    m_log.setValue("FirmwareVersion", m_unit.stats().firmware);

    connect(&m_ticker, &QTimer::timeout, this,
            [=](){
        switch (m_state) {
        case State::NONE :
            m_unit.setCharge(false);
            m_passiveWatch.start();
            m_state = State::PASSIVE;
            break;

        case State::PASSIVE :
            if (m_passiveWatch.elapsed() > INERTIA_DELAY) {
                sampleData();
            }
            if (m_passiveWatch.elapsed() > s_duration.passive) {
                emit finished(m_state, evaluate());
                m_log.setValue("MeasuringDuration_noCharge", s_duration.passive);
                m_unit.setCharge(true);
                m_pairingWatch.start();
                m_state = State::PAIRING;
            }
            break;

        case State::PAIRING :
            if (m_unit.isPaired()) {
                emit finished(m_state, true);
                m_log.setValue("PairingElapsed", m_pairingWatch.elapsed());
                m_activeWatch.start();
                m_state = State::ACTIVE;
            } else if (m_pairingWatch.elapsed() > s_duration.pairing) {
                emit finished(m_state, false, QString("Pairing timed out!"));
                m_OK = false;
                m_log.setValue("PairingElapsed", s_duration.pairing);
                m_state = State::DONE;
            } else {
                m_unit.pair();
            }
            break;

        case State::ACTIVE :
            if (m_activeWatch.elapsed() > INERTIA_DELAY) {
                sampleData();
            }
            if (m_activeWatch.elapsed() > s_duration.active) {
                emit finished(m_state, evaluate());
                m_log.setValue("MeasurigDuration_charge", s_duration.active);
                if (m_unit.stats().wing.dataPresent) {
                    m_log.setValue("WingSerial", m_unit.stats().wing.serial);
                    m_log.setValue("WingBatCapacity", m_unit.stats().wing.batCapacity);
                    m_log.setValue("WingBatVolt", m_unit.stats().wing.batVolt);
                    m_log.setValue("WingHumidity", m_unit.stats().wing.humidity);
                    m_log.setValue("WingTemperature", m_unit.stats().wing.temperature);
                }
                m_state = State::DONE;
            }
            break;

        case State::DONE :
            emit finished(m_state, m_OK, m_feedback);
            stop();
            m_log.setValue("TestVersion", VERSION);
            m_log.setValue("SlotTemperature", m_unit.stats().temperature);
            m_log.setValue("Approved", m_OK);
            m_log.save();
            break;
        }
    });
}

void EscFuncTest::start(const int interval)
{
    if (m_ticker.isActive()) {
        m_ticker.setInterval(interval);
    } else {
        m_ticker.start(interval);
    }
}

void EscFuncTest::stop()
{
    if (m_ticker.isActive()) {
        m_ticker.stop();
    }
    m_state = State::NONE;
}

bool EscFuncTest::isRunning() const
{
    return m_state != State::NONE && m_state != State::DONE;
}

void EscFuncTest::setLimits(const EscFuncTest::Limits &limits)
{
    s_limit = limits;
}

EscFuncTest::Limits EscFuncTest::getLimits()
{
    return s_limit;
}

void EscFuncTest::setDuration(EscFuncTest::Durations duration)
{
    s_duration = duration;
}

EscFuncTest::Durations EscFuncTest::getDuration()
{
    return s_duration;
}

void EscFuncTest::sampleData()
{
    m_iSupplySamples.push_back(m_unit.stats().iSupply);
    m_iSupplyWingSamples.push_back(m_unit.stats().iSupplyWing);
    if (m_state == State::ACTIVE) {
        if (m_unit.stats().wing.dataPresent) {
            m_slotLQISamples.push_back(m_unit.stats().LQI);
            m_wingLQISamples.push_back(m_unit.stats().wing.LQI);
            m_wingLossSamples.push_back(m_unit.stats().wing.loss);
        } else {
            emit finished(m_state, false, QString("[#%0] lost connection to wing!").arg(m_unit.id()));
            emit finished(State::DONE, false);
            stop();
        }
    }
}

bool EscFuncTest::evaluate()
{
    bool approved = true;
    if (m_state != State::PASSIVE && m_state != State::ACTIVE) {
        emit error(QString("Internal error; evaluation on illegal state!"));
        stop();
        return false;
    }

    if (m_state == State::ACTIVE) {
        if (!approveRadio())
            approved = false;
        if (!approveCharging())
            approved = false;
    }

    if (!approvePower()) {
        approved = false;
    }

    if (!approved) {
        m_OK = false;
    }
    return approved;
}

bool EscFuncTest::approveRadio()
{
    bool approved = true;
    auto slotLQI = std::accumulate(m_slotLQISamples.begin(), m_slotLQISamples.end(), 0.0) / m_slotLQISamples.size();
    m_log.setValue("SlotLqi", slotLQI);
    m_slotLQISamples.clear();
    if (slotLQI < s_limit.slotLQI) {
        approved = false;
        m_feedback.append(QString("\n[#%0] read slotLQI[%1] (<%2)")
                        .arg(m_unit.id())
                        .arg(slotLQI)
                        .arg(s_limit.slotLQI));
    }
    auto slotLoss = m_unit.stats().loss;
    m_log.setValue("SlotLoss", slotLoss);
    if (slotLoss > s_limit.slotLoss) {
        approved = false;
        m_feedback.append(QString("\n[#%0] read slotLoss[%1] (>%2)")
                        .arg(m_unit.id())
                        .arg(slotLoss)
                        .arg(s_limit.slotLoss));
    }
    auto wingLQI = std::accumulate(m_wingLQISamples.begin(), m_wingLQISamples.end(), 0.0) / m_wingLQISamples.size();
    m_log.setValue("WingLQI", wingLQI);
    m_wingLQISamples.clear();
    if (wingLQI < s_limit.wingLQI) {
        approved = false;
        m_feedback.append(QString("\n[#%0] read wingLQI[%1] (<%2)")
                        .arg(m_unit.id())
                        .arg(wingLQI)
                        .arg(s_limit.wingLQI));
    }
    auto wingLoss = std::accumulate(m_wingLossSamples.begin(), m_wingLossSamples.end(), 0.0) / m_wingLossSamples.size();
    m_log.setValue("WingLoss", wingLoss);
    m_wingLossSamples.clear();
    if (wingLoss > s_limit.wingLoss) {
        approved = false;
        m_feedback.append(QString("\n[#%0] read wingLoss[%1] (>%2)")
                        .arg(m_unit.id())
                        .arg(wingLoss)
                        .arg(s_limit.wingLoss));
    }
    return approved;
}

bool EscFuncTest::approveCharging()
{
    bool approved = true;
    if (m_unit.stats().wing.batCurrent < s_limit.chargeCurent) {
        approved = false;
        m_feedback.append(QString("\n[#%0] read battery current[%1] (<%2)")
                        .arg(m_unit.id())
                        .arg(m_unit.stats().wing.batCurrent)
                        .arg(s_limit.chargeCurent));
    }
    m_log.setValue("WingBatCurrent", m_unit.stats().wing.batCurrent);
    return approved;
}

bool EscFuncTest::approvePower()
{
    bool approved = true;
    auto iSupply = std::accumulate(m_iSupplySamples.begin(), m_iSupplySamples.end(), 0.0) / m_iSupplySamples.size();
    m_iSupplySamples.clear();
    if (iSupply > (double)((m_state == State::PASSIVE) ? s_limit.iSupply_passive : s_limit.iSupply_active)) {
        approved = false;
        m_feedback.append(QString("\n[#%0] read iSupply_%1[%2] (>%3)")
                        .arg(m_unit.id())
                        .arg((m_state == State::PASSIVE) ? "passive" : "active")
                        .arg(iSupply)
                        .arg((m_state == State::PASSIVE) ? s_limit.iSupply_passive : s_limit.iSupply_active));
    }
    auto iSupplyWing = std::accumulate(m_iSupplyWingSamples.begin(), m_iSupplyWingSamples.end(), 0.0) / m_iSupplyWingSamples.size();
    m_iSupplyWingSamples.clear();
    if (iSupplyWing > (double)((m_state == State::PASSIVE) ? s_limit.iSupplyWing_passive : s_limit.iSupplyWing_active)) {
        approved = false;
        m_feedback.append(QString("\n[#%0] read iSupplyWing_%1[%2] (>%3)")
                        .arg(m_unit.id())
                        .arg((m_state == State::PASSIVE) ? "passive" : "active")
                        .arg(iSupplyWing)
                        .arg((m_state == State::PASSIVE) ? s_limit.iSupplyWing_passive : s_limit.iSupplyWing_active));
    }
    m_log.setValue(QString("iSupply_%0").arg((m_state == State::PASSIVE) ? "noCharge" : "charge"), m_unit.stats().iSupply);
    m_log.setValue(QString("iSupplyWing_%0").arg((m_state == State::PASSIVE) ? "noCharge" : "charge"), m_unit.stats().iSupplyWing);
    return approved;
}
