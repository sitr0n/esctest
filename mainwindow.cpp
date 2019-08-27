#include "mainwindow.h"
#include <QSpacerItem>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QRegExp>
#include <QToolTip>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : ToolFrame(parent)
    , m_toggleButton(new QPushButton(this))
{
    setWindowTitle("eBird WCB Manager");
    useEventlog();
    putContent(makeContent());
    putSettings(makeSettings());

    findCommunicationServer(0, 0);
    connect(this, &ToolFrame::start, this,
            [=](){
        findCommunicationServer(0, 0);
    });

    m_unitEditor.setEnabled(false);
    m_recordPanel.setEnabled(false);
    m_testPanel.setEnabled(false);

    output() << QString("Test version %1").arg(EscFuncTest::VERSION);
}


void MainWindow::startTest()
{
    if (m_test_queue.size() < 1) {
        return;
    }
    resetTest();
    if (m_test != nullptr) {
        delete m_test;
    }

    auto test_unit = m_test_queue.front();
    m_test_queue.pop();
    m_test = new EscFuncTest(*test_unit, this);
    if (false) { // TODO: give user a looping option
        m_test_queue.push(test_unit);
    }

    const bool isMutable = false;
    m_unitBrowser.display(test_unit, isMutable);

    connect(m_test, &EscFuncTest::finished, this,
            [=](const EscFuncTest::State &state, const bool &passed, const QString &message){
        switch (state) {
        case EscFuncTest::State::NONE :
            output() << QString("Test finished state: NONE");
            break;

        case EscFuncTest::State::PASSIVE :
            m_progressBar.reset();
            m_progressBar.glideTo(100, EscFuncTest::getDuration().pairing);
            if (passed) {
                m_testPassiveLED->setGreen();
            } else {
                m_testPassiveLED->setRed();
            }
            break;

        case EscFuncTest::State::PAIRING :
            m_progressBar.reset();
            m_progressBar.glideTo(100, EscFuncTest::getDuration().active);
            if (passed) {
                m_testPairingLED->setGreen();
            } else {
                m_testPairingLED->setRed();
            }
            break;

        case EscFuncTest::State::ACTIVE :
            if (passed) {
                m_testActiveLED->setGreen();
            } else {
                m_testActiveLED->setRed();
            }
            break;

        case EscFuncTest::State::DONE :
            m_progressBar.setValue(100);
            if (!m_test_queue.empty()) {
                QTimer::singleShot(TESTING_DELAY, this, [=](){
                    startTest();
                });
            } else {
                m_testButton.setState("Test");
                m_busScanner.setEnabled(true);
                m_unitEditor.setEnabled(true);
                started();
            }
            break;
        }
        if (!message.isEmpty()) {
            output() << message;
        }
    });

    connect(m_test, &EscFuncTest::error, this,
            [=](const QString &message){
        error();
        output() << message;
    });

    m_test->start(TESTING_INTERVAL);
    m_progressBar.glideTo(100, EscFuncTest::getDuration().passive);
}

void MainWindow::resetTest()
{
    m_progressBar.setValue(0);
    m_testPassiveLED->setInactive();
    m_testPairingLED->setInactive();
    m_testActiveLED->setInactive();
}

QWidget *MainWindow::makeContent()
{
    auto layout = new QFormLayout(&m_content);
    layout->addRow(makeBusScanner());
    layout->addRow(makeUnitViewer());
    layout->addRow(makeRecordPanel());
    layout->addRow(makeTestPanel());
    layout->addRow(&m_unitBrowser);
    return &m_content;
}

QWidget *MainWindow::makeBusScanner()
{
    auto busViewer = new QCheckDrop(this);
    auto scanButton = new QPushButton("Scan", this);

    busViewer->putHeader("--- Select channels ---");
    for (int i = 1; i <= NUM_BUSSES; ++i) {
        busViewer->addRow(i, QString("Channel %0").arg(i));
    }

    connect(scanButton, &QPushButton::clicked, this,
            [=](){
        for (const auto& bus: busViewer->checkedItems()) {
            m_units = WingSlot::discoverUnits(bus);
        }

        m_unitViewer.clear();
        for (const WingSlot& unit: m_units) {
            m_unitViewer.addRow(unit.id(), QString("#%0 - Firmware [%1]").arg(unit.id()).arg(unit.stats().firmware));
        }

        m_toggleButton->setText(QString("%0 units").arg(m_units.size()));
        m_unitEditor.setEnabled(!m_units.empty());
        m_testPanel.setEnabled(!m_units.empty());
    });

    auto layout = new QFormLayout(&m_busScanner);
    layout->addRow(busViewer, scanButton);
    return &m_busScanner;
}

QWidget *MainWindow::makeUnitViewer()
{
    connect(m_toggleButton, &QPushButton::clicked, this,
            [=](){
        m_unitViewer.toggleRows();
    });

    connect(&m_unitViewer, &QCheckView::selected, this,
            [=](const int id){
        if (m_test != nullptr && m_test->isRunning()) {
            return;
        }
        auto unit = std::find_if(m_units.begin(), m_units.end(),
                [&](WingSlot::Unit &u){
            return (u.get().id() == id);
        });
        const bool isMutable = true;
        m_unitBrowser.display(&unit->get(), isMutable);
    });

    auto layout = new QFormLayout(&m_unitEditor);
    layout->addRow(m_toggleButton);
    layout->addRow(&m_unitViewer);
    return &m_unitEditor;
}

QWidget *MainWindow::makeRecordPanel()
{
    auto addRecordButton = new QPushButton("+", this);
    auto recordViewer = new QCheckDrop(this);
    auto rmRecordButton = new QPushButton("-", this);
    auto spacer = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    auto recordLED = new StatusBitWidget(this);

    recordViewer->putHeader("--- Units recording ---");
    recordViewer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    recordLED->setFixedSize(QSize(LED_SIZE, LED_SIZE));
    recordLED->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    connect(addRecordButton, &QPushButton::clicked, this,
            [=](){
        if (m_recorder != nullptr) {
            delete m_recorder;
        }
        auto selection = m_unitViewer.checkedItems();
        WingSlot::SlotList units;
        auto it = m_units.begin();
        do {
            it = std::find_if(it, m_units.end(), [&](WingSlot::Unit &u){
                bool match = false;
                for (const auto& id: selection) {
                    if (u.get().id() == id) {
                        match = true;
                    }
                }
                return match;
            });
            if (it != m_units.end()) {
                if (true) { // TODO: IF NOT DUPLICATE
                    units.push_back(*it);
                    recordViewer->addRow(it->get().id(), QString("#%0").arg(it->get().id()));
                }
                ++it;
            }
        } while (it != m_units.end());

        m_recorder = new EscRecorder(units, this);
        m_recorder->setPath("~/PALM/log");
        m_recorder->start(RECORDING_INTERVAL);
    });

    connect(rmRecordButton, &QPushButton::clicked, this,
            [=](){
        if (m_recorder != nullptr) {
            m_recorder->stop();
            delete m_recorder;
            m_recorder = nullptr;
        }

        auto selection = recordViewer->uncheckedItems();
        recordViewer->clear();
        WingSlot::SlotList units;
        auto it = m_units.begin();
        do {
            it = std::find_if(it, m_units.end(), [&](WingSlot::Unit &u){
                bool match = false;
                for (const auto& id: selection) {
                    if (u.get().id() == id) {
                        match = true;
                    }
                }
                return match;
            });
            if (it != m_units.end()) {
                units.push_back(*it);
                recordViewer->addRow(it->get().id(), QString("#%0").arg(it->get().id()));
                ++it;
            }
        } while (it != m_units.end());
        output() << QString("[%1] many items left").arg(units.size());
        if (!units.empty()) {
            m_recorder = new EscRecorder(units, this);
        }
    });

    auto layout = new QHBoxLayout(&m_recordPanel);
    layout->addWidget(addRecordButton);
    layout->addWidget(recordViewer);
    layout->addWidget(rmRecordButton);
    layout->addItem(spacer);
    layout->addWidget(recordLED);
    return &m_recordPanel;
}

QWidget *MainWindow::makeTestPanel()
{
    m_testPassiveLED = new StatusBitWidget(this);
    m_testPairingLED = new StatusBitWidget(this);
    m_testActiveLED = new StatusBitWidget(this);

    m_testButton.addState(QString("Test"));
    m_testButton.addState(QString("Stop"));
    m_testPassiveLED->setFixedSize(QSize(LED_SIZE, LED_SIZE));
    m_testPassiveLED->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_testPairingLED->setFixedSize(QSize(LED_SIZE, LED_SIZE));
    m_testPairingLED->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_testActiveLED->setFixedSize(QSize(LED_SIZE, LED_SIZE));
    m_testActiveLED->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    connect(&m_testButton, &QStateButton::state_change, this,
            [=](const QString &state){
        if (QString::compare(state, "Test") == 0) {
            auto selection = m_unitViewer.checkedItems();
            auto it = m_units.begin();
            do {
                it = std::find_if(it, m_units.end(), [&](WingSlot::Unit &u){
                    bool match = false;
                    for (const auto& id: selection) {
                        if (u.get().id() == id) {
                            match = true;
                        }
                    }
                    return match;
                });
                if (it != m_units.end()) {
                    m_test_queue.push(&it->get());
                    ++it;
                }
            } while (it != m_units.end());

            if (m_test_queue.empty()) {
                auto index = m_unitViewer.currentIndex().row();
                auto unit = &(index >= 0 ? m_units.at(index) : m_units.front()).get();
                m_test_queue.push(unit);
            }

            startTest();
            m_busScanner.setEnabled(false);
            running();
        } else {
            m_test->stop();
            m_progressBar.pause();
            m_busScanner.setEnabled(true);
            m_unitEditor.setEnabled(true);
            started();
        }

    });

    auto layout = new QHBoxLayout(&m_testPanel);
    layout->addWidget(&m_testButton);
    layout->addWidget(&m_progressBar);
    layout->addWidget(m_testPassiveLED);
    layout->addWidget(m_testPairingLED);
    layout->addWidget(m_testActiveLED);
    return &m_testPanel;
}

QWidget *MainWindow::makeSettings()
{
    auto settings = new QFormLayout();
    settings->addRow(tr("&Default settings"), &reset_button);
    reset_button.setText("Reset");
    connect(&reset_button, &QPushButton::clicked, this,
            [=](){
        output() << QString("Settings reset");
        saveSettings("passive_duration", TEST_DEFAULT_PASSIVE_DURATION);
        passive_duration_edit.setText(QString::number(TEST_DEFAULT_PASSIVE_DURATION));

        saveSettings("pairing_duration", TEST_DEFAULT_PAIRING_DURATION);
        pairing_duration_edit.setText(QString::number(TEST_DEFAULT_PAIRING_DURATION));

        saveSettings("active_duration", TEST_DEFAULT_ACTIVE_DURATION);
        active_duration_edit.setText(QString::number(TEST_DEFAULT_ACTIVE_DURATION));

        saveSettings("iSupply_noCharge_edit", iSupply_noCharge_value);
        iSupply_noCharge_edit.setText(QString::number(iSupply_noCharge_value));

        saveSettings("iSupply_charge_edit", iSupply_charge_value);
        iSupply_charge_edit.setText(QString::number(iSupply_charge_value));

        saveSettings("iSupplyWing_noCharge_edit", iSupplyWing_noCharge_value);
        iSupplyWing_noCharge_edit.setText(QString::number(iSupplyWing_noCharge_value));

        saveSettings("iSupplyWing_charge_edit", iSupplyWing_charge_value);
        iSupplyWing_charge_edit.setText(QString::number(iSupplyWing_charge_value));

        saveSettings("wingLoss_edit", wingLoss_value);
        wingLoss_edit.setText(QString::number(wingLoss_value));

        saveSettings("wingChargeCurrent_edit", wingChargeCurrent_value);
        wingChargeCurrent_edit.setText(QString::number(wingChargeCurrent_value));

        saveSettings("packetLoss_edit", packetLoss_value);
        packetLoss_edit.setText(QString::number(packetLoss_value));

        loadSettings();
    });

    settings->addRow(tr("&Passive duration"), &passive_duration_edit);
    passive_duration_edit.setPlaceholderText(QString("[ms]"));
    connect(&passive_duration_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = passive_duration_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(passive_duration_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 10000.0;
        const double MAX_NUMBER = 60000.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(passive_duration_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("passive_duration", number);
        loadSettings();
    });

    settings->addRow(tr("&Pairing duration"), &pairing_duration_edit);
    pairing_duration_edit.setPlaceholderText(QString("[ms]"));
    connect(&pairing_duration_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = pairing_duration_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(pairing_duration_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 10000.0;
        const double MAX_NUMBER = 60000.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(pairing_duration_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("pairing_duration", number);
        loadSettings();
    });

    settings->addRow(tr("&Active duration"), &active_duration_edit);
    active_duration_edit.setPlaceholderText(QString("[ms]"));
    connect(&active_duration_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = active_duration_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(active_duration_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 10000.0;
        const double MAX_NUMBER = 60000.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(active_duration_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("active_duration", number);
        loadSettings();
    });
    settings->addRow(tr("&Max input current (passive)"), &iSupply_noCharge_edit);
    iSupply_noCharge_edit.setPlaceholderText("[mA]");
    connect(&iSupply_noCharge_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = iSupply_noCharge_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(iSupply_noCharge_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 1.0;
        const double MAX_NUMBER = 1000.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(iSupply_noCharge_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("iSupply_noCharge_edit", number);
        loadSettings();
    });
    settings->addRow(tr("&Max input current (active)"), &iSupply_charge_edit);
    iSupply_charge_edit.setPlaceholderText("[mA]");
    connect(&iSupply_charge_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = iSupply_charge_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(iSupply_charge_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 1.0;
        const double MAX_NUMBER = 1000.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(iSupply_charge_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("iSupply_charge_edit", number);
        loadSettings();
    });
    settings->addRow(tr("&Max output current (passive)"), &iSupplyWing_noCharge_edit);
    iSupplyWing_noCharge_edit.setPlaceholderText("[mA]");
    connect(&iSupplyWing_noCharge_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = iSupplyWing_noCharge_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(iSupplyWing_noCharge_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 1.0;
        const double MAX_NUMBER = 1000.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(iSupplyWing_noCharge_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("iSupplyWing_noCharge_edit", number);
        loadSettings();
    });
    settings->addRow(tr("&Max output current (active)"), &iSupplyWing_charge_edit);
    iSupplyWing_charge_edit.setPlaceholderText("[mA]");
    connect(&iSupplyWing_charge_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = iSupplyWing_charge_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(iSupplyWing_charge_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 1.0;
        const double MAX_NUMBER = 1000.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(iSupplyWing_charge_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("iSupplyWing_charge_edit", number);
        loadSettings();
    });
    settings->addRow(tr("&Max wing loss"), &wingLoss_edit);
    wingLoss_edit.setPlaceholderText("[%]");
    connect(&wingLoss_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = wingLoss_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(wingLoss_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 0.0;
        const double MAX_NUMBER = 100.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(wingLoss_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("wingLoss_edit", number);
        loadSettings();
    });
    settings->addRow(tr("&Min wing charge current"), &wingChargeCurrent_edit);
    connect(&wingChargeCurrent_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = wingChargeCurrent_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(wingChargeCurrent_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 0.0;
        const double MAX_NUMBER = 52.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(wingChargeCurrent_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("wingChargeCurrent_edit", number);
        loadSettings();
    });
    settings->addRow(tr("&Max packet loss"), &packetLoss_edit);
    packetLoss_edit.setPlaceholderText("[%]");
    connect(&packetLoss_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = packetLoss_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(packetLoss_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 0.0;
        const double MAX_NUMBER = 100.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(packetLoss_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("packetLoss_edit", number);
        loadSettings();
    });

    auto settings_wrapper = new QWidget(this);
    settings_wrapper->setLayout(settings);

    loadSettings();

    return settings_wrapper;
}

void MainWindow::loadSettings()
{
    QSettings settings("Seatex", "WingSlotTest");
    auto test_limits = EscFuncTest::getLimits();
    auto test_durations = EscFuncTest::getDuration();

    auto passive_duration_val = settings.value(QString("passive_duration"), TEST_DEFAULT_PASSIVE_DURATION);
    passive_duration_edit.setText(passive_duration_val.toString());
    test_durations.passive = passive_duration_val.toInt();

    auto pairing_duration_val = settings.value(QString("pairing_duration"), TEST_DEFAULT_PAIRING_DURATION);
    pairing_duration_edit.setText(pairing_duration_val.toString());
    test_durations.pairing = pairing_duration_val.toInt();

    auto active_duration_val = settings.value(QString("active_duration"), TEST_DEFAULT_ACTIVE_DURATION);
    active_duration_edit.setText(active_duration_val.toString());
    test_durations.active = active_duration_val.toInt();

    auto iSupply_noCharge_edit_val = settings.value(QString("iSupply_noCharge_edit"), iSupply_noCharge_value);
    iSupply_noCharge_edit.setText(iSupply_noCharge_edit_val.toString());
    test_limits.iSupply_passive = iSupply_noCharge_edit_val.toFloat();

    auto iSupply_charge_edit_val = settings.value(QString("iSupply_charge_edit"), iSupply_charge_value);
    iSupply_charge_edit.setText(iSupply_charge_edit_val.toString());
    test_limits.iSupply_active = iSupply_charge_edit_val.toFloat();

    auto iSupplyWing_noCharge_edit_val = settings.value(QString("iSupplyWing_noCharge_edit"), iSupplyWing_noCharge_value);
    iSupplyWing_noCharge_edit.setText(iSupplyWing_noCharge_edit_val.toString());
    test_limits.iSupplyWing_passive = iSupplyWing_noCharge_edit_val.toFloat();

    auto iSupplyWing_charge_edit_val = settings.value(QString("iSupplyWing_charge_edit"), iSupplyWing_charge_value);
    iSupplyWing_charge_edit.setText(iSupplyWing_charge_edit_val.toString());
    test_limits.iSupplyWing_active = iSupplyWing_charge_edit_val.toFloat();

    auto wingLoss_edit_val = settings.value(QString("wingLoss_edit"), wingLoss_value);
    wingLoss_edit.setText(wingLoss_edit_val.toString());
    test_limits.wingLoss = wingLoss_edit_val.toFloat();

    auto wingChargeCurrent_edit_val = settings.value(QString("wingChargeCurrent_edit"), wingChargeCurrent_value);
    wingChargeCurrent_edit.setText(wingChargeCurrent_edit_val.toString());
    test_limits.chargeCurent = wingChargeCurrent_edit_val.toFloat();

    auto packetLoss_edit_val = settings.value(QString("packetLoss_edit"), packetLoss_value);
    packetLoss_edit.setText(packetLoss_edit_val.toString());
    test_limits.slotLoss = packetLoss_edit_val.toFloat();

    EscFuncTest::setLimits(test_limits);
    EscFuncTest::setDuration(test_durations);
}

void MainWindow::saveSettings(const QString &item, QVariant value)
{
    QSettings settings("Seatex", "WingSlotTest");
    settings.setValue(item, value);
}

bool MainWindow::isNumber(const QString &item)
{
    QRegExp format("^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)$");
    return format.exactMatch(item);
}

void MainWindow::findCommunicationServer(int argc, char** argv)
{
    if (WingSlot::findCommunicationServer(argc, argv)) {
        WingSlot::setProcessingLoad(LOAD_FACTOR);
        started();
    } else {
        output() << QString("Could not find the esvr");
        error();
    }
}

