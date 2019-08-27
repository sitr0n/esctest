#include "escmonitor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDebug>

EscMonitor::EscMonitor(QWidget *parent)
    : QWidget(parent)
    , m_unit(nullptr)
    , m_charging(false)
    , m_isMutable(true)
    , m_chargingButton(new QPushButton("Charge", this))
    , m_chargingLED(new StatusBitWidget(this))
    , m_pairingButton(new QPushButton("Pair", this))
    , m_pairingLED(new StatusBitWidget(this))
    , m_dataGraph(new GraphWidget())
{
    m_stopwatch.start();
    m_dataGraph->setTheme(GraphStyler::DarkTheme);
    setLayout(new QVBoxLayout(this));
    layout()->addWidget(&m_dataPanel);
    auto stats_wrapper = new QWidget(this);
    stats_wrapper->setLayout(new QHBoxLayout());
    stats_wrapper->layout()->addWidget(&m_slotPanel);
    stats_wrapper->layout()->addWidget(&m_wingPanel);
    layout()->addWidget(stats_wrapper);

    auto slot_layout = new QFormLayout();
    slot_layout->addRow(new QLabel("<b>Slot</b>"));
    slot_layout->addRow(tr("Serial"), &m_slotSerial);
    m_slotSerial.setText("N/A");
    slot_layout->addRow(tr("Temperature"), &m_slotTemperature);
    m_slotTemperature.setText("N/A");
    slot_layout->addRow(tr("LQI"), &m_slotLQI);
    m_slotLQI.setText("N/A");
    slot_layout->addRow(tr("Loss"), &m_slotLoss);
    m_slotLoss.setText("N/A");
    slot_layout->addRow(tr("Current"), &m_slotCurrent);
    m_slotCurrent.setText("N/A");
    slot_layout->addRow(m_chargingButton, m_chargingLED);
    m_slotPanel.setLayout(slot_layout);

    auto wing_layout = new QFormLayout();
    wing_layout->addRow(new QLabel("<b>Wing</b>"));
    wing_layout->addRow(tr("Serial"), &m_wingSerial);
    m_wingSerial.setText("N/A");
    wing_layout->addRow(tr("Temperature"), &m_wingTemperature);
    m_wingTemperature.setText("N/A");
    wing_layout->addRow(tr("LQI"), &m_wingLQI);
    m_wingLQI.setText("N/A");
    wing_layout->addRow(tr("Loss"), &m_wingLoss);
    m_wingLoss.setText("N/A");
    wing_layout->addRow(tr("Current"), &m_wingCurrent);
    m_wingCurrent.setText("N/A");
    wing_layout->addRow(m_pairingButton, m_pairingLED);
    m_wingPanel.setLayout(wing_layout);

    auto data_layout = new QVBoxLayout();
    data_layout->addWidget(m_dataGraph);
    m_dataPanel.setLayout(data_layout);

    m_chargingLED->setFixedSize(QSize(LED_SIZE, LED_SIZE));
    m_chargingLED->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pairingLED->setFixedSize(QSize(LED_SIZE, LED_SIZE));
    m_pairingLED->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

}

void EscMonitor::display(WingSlot *unit, bool editable)
{
    m_isMutable = editable;
    m_chargingButton->setEnabled(m_isMutable);
    m_pairingButton->setEnabled(m_isMutable);
    if (m_unit != nullptr) {
        disconnect(m_unit, &WingSlot::new_data, this, &EscMonitor::displayData);
    }
    m_unit = unit;
    connect(m_unit, &WingSlot::new_data, this, &EscMonitor::displayData);

    m_slotSerial.setText(QString("#%0").arg(unit->id()));

    if (editable) {
        connect(m_chargingButton, &QPushButton::clicked, this,
                [=](){
            unit->setCharge(!m_charging);

        });

        connect(m_pairingButton, &QPushButton::clicked, this,
                [=](){
            unit->pair();
        });

    } else {
        m_chargingButton->setEnabled(false);
        m_pairingButton->setEnabled(false);
    }
}

void EscMonitor::displayData(const WingSlot::Stats &stats)
{
    m_slotTemperature.setText(QString("%0 °C").arg(QString::number(stats.temperature, 'f', DATA_DECIMALS)));
    m_slotLQI.setText(QString("%0%").arg(QString::number(stats.LQI, 'f', DATA_DECIMALS).remove(QRegExp("\\.?0+$"))));
    m_slotLoss.setText(QString("%0%").arg(QString::number(stats.loss, 'f', DATA_DECIMALS).remove(QRegExp("\\.?0+$"))));
    m_slotCurrent.setText(QString("%0 mA").arg(QString::number(stats.iSupply, 'f', DATA_DECIMALS).remove(QRegExp("\\.?0+$"))));
    m_wingCurrent.setText(QString("%0 mA").arg(QString::number(stats.iSupplyWing, 'f', DATA_DECIMALS).remove(QRegExp("\\.?0+$"))));

    if (m_unit->isPaired()) {
        m_wingSerial.setText(QString("#%0").arg(stats.wing.serial));
        m_wingTemperature.setText(QString("%0 °C").arg(QString::number(stats.wing.temperature, 'f', DATA_DECIMALS)));
        m_wingLQI.setText(QString("%0%").arg(QString::number(stats.wing.LQI, 'f', DATA_DECIMALS).remove(QRegExp("\\.?0+$"))));
        m_wingLoss.setText(QString("%0%").arg(QString::number(stats.wing.loss, 'f', DATA_DECIMALS).remove(QRegExp("\\.?0+$"))));
    } else {
        m_wingSerial.setText("N/A");
        m_wingTemperature.setText("N/A");
        m_wingLQI.setText("N/A");
        m_wingLoss.setText("N/A");
    }


    m_dataGraph->addData((double)m_stopwatch.elapsed()/1000, stats.iSupply);
    if (m_unit->isPaired()) {
        m_pairingLED->setGreen();
    } else {
        m_pairingLED->setInactive();
    }
    if (!m_charging && m_unit->isCharging()) {
        m_charging = true;
        m_chargingLED->setGreen();
        if (m_isMutable) {
            m_pairingButton->setEnabled(true);
        }

    }
    if (m_charging && !m_unit->isCharging()) {
        m_charging = false;
        m_chargingLED->setInactive();
        m_pairingButton->setEnabled(false);
    }
}


