#include "mainwindow.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QRegExp>
#include <QToolTip>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : ToolFrame(parent)
{
    setWindowTitle("eBird WingSlot Test");
    test.putTextStream(&output());
    useEventlog();
    putContent(makeContent());
    putSettings(makeSettings());

    output() << QString("Test version %1").arg(test.getTestVersion());
}

QWidget *MainWindow::makeContent()
{
    auto wrapper = new QWidget(this);
    wrapper->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    auto layout = new QFormLayout(wrapper);
    layout->addRow(&test_info, &start_button);
    start_button.setMinimumSize(QSize(BUTTON_SIZE/2, BUTTON_SIZE/2));
    start_button.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    start_button.addState("Start");
    start_button.addState("Stop");
    start_button.setEnabled(false);
    layout->addRow(&progress_bar);
    progress_bar.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    layout->addRow(tr("&Passive power"), &idle_power_status);
    idle_power_status.setFixedSize(QSize(BUTTON_SIZE/2.5, BUTTON_SIZE/2.5));
    idle_power_status.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addRow(tr("&Active power"), &active_power_status);
    active_power_status.setFixedSize(QSize(BUTTON_SIZE/2.5, BUTTON_SIZE/2.5));
    active_power_status.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addRow(tr("&Radio communication"), &radio_com_status);
    radio_com_status.setFixedSize(QSize(BUTTON_SIZE/2.5, BUTTON_SIZE/2.5));
    radio_com_status.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addRow(tr("&Charge capability"), &charging_status);
    charging_status.setFixedSize(QSize(BUTTON_SIZE/2.5, BUTTON_SIZE/2.5));
    charging_status.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addRow(tr("&Packet loss"), &packet_loss_status);
    packet_loss_status.setFixedSize(QSize(BUTTON_SIZE/2.5, BUTTON_SIZE/2.5));
    packet_loss_status.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(&start_button, &QStateButton::state_change, this,
            [=](const QString &state){
        if (QString::compare(state, QString("Start")) == 0) {
            output() << QString("Starting test");
            progress_bar.reset();
            idle_power_status.setInactive();
            active_power_status.setInactive();
            radio_com_status.setInactive();
            packet_loss_status.setInactive();
            charging_status.setInactive();
            test.start();
            running();
            progress_bar.glideTo(40, 15000);
        } else if (QString::compare(QString("Stop"), state) == 0) {
            output() << QString("Stopping test");
            test.stop();
            resetDisplay();
        } else {
            output() << QString("StateButton emitted unexpected state [%1]").arg(state);
        }
    });

    connect(this, &ToolFrame::start, this,
            [=](){
        if (!test.isRunning()) {
            resetDisplay();
            auto connections = test.findUnits();
            if (connections > 0) {
                started();
                start_button.setEnabled(true);
            } else {
                error();
                start_button.setEnabled(false);
            }
            QString units_str = (connections == 1) ? "unit" : "units";
            test_info.setText(QString("<b>Test %1 %2</b>").arg(connections).arg(units_str));
            output() << QString("Found %1 connected %2").arg(connections).arg(units_str);
        }
    });

    connect(&test, &FunctionTest::idle_power_test, this,
            [=](bool approved){
        if (approved) {
            idle_power_status.setGreen();
        } else {
            idle_power_status.setRed();
        }
    });

    connect(&test, &FunctionTest::wings_found, this,
            [=](){
        QSettings settings("Seatex", "WingSlotTest");
        auto duration = settings.value(QString("measuring_duration"), 10000);
        progress_bar.glideTo(99, duration.toInt());
    });

    connect(&test, &FunctionTest::active_power_test, this,
            [=](bool approved){
        if (approved) {
            active_power_status.setGreen();
        } else {
            active_power_status.setRed();
        }
    });

    connect(&test, &FunctionTest::radio_com_test, this,
            [=](bool approved){
        if (approved) {
            radio_com_status.setGreen();
        } else {
            radio_com_status.setRed();
        }
    });

    connect(&test, &FunctionTest::charge_test, this,
            [=](bool approved){
        if (approved) {
            charging_status.setGreen();
        } else {
            charging_status.setRed();
        }
    });

    connect(&test, &FunctionTest::packet_loss_test, this,
            [=](bool approved){
        if (approved) {
            packet_loss_status.setGreen();
        } else {
            packet_loss_status.setRed();
        }
        progress_bar.setValue(100);
        output() << QString("Finished test\n");
        resetDisplay();
    });

    connect(&test, &FunctionTest::error, this,
            [=](const QString &message){
        output() << message;
        error();
    });

    return wrapper;
}

void MainWindow::refreshTestUnits()
{
    auto connected = test.findUnits();
    test_info.setText(QString("<b>Test %1 device[s]</b>").arg(connected));
}

void MainWindow::resetDisplay()
{
    stopped();
    start_button.setState("Start");
    start_button.setEnabled(false);
}

QWidget *MainWindow::makeSettings()
{
    auto settings = new QFormLayout();
    settings->addRow(tr("&Default settings"), &reset_button);
    reset_button.setText("Reset");
    connect(&reset_button, &QPushButton::clicked, this,
            [=](){
        output() << QString("Settings reset");
        const double measuring_duration_value = 10000;
        saveSettings("measuring_duration", measuring_duration_value);
        test.setMeasuringDuration(measuring_duration_value);
        measuring_duration.setText(QString::number(measuring_duration_value));

        const double iSupply_noCharge_value = 8;
        saveSettings("iSupply_noCharge_edit", iSupply_noCharge_value);
        test.setMaxISupply_noCharge(iSupply_noCharge_value);
        iSupply_noCharge_edit.setText(QString::number(iSupply_noCharge_value));

        const double iSupply_charge_value = 180;
        saveSettings("iSupply_charge_edit", iSupply_charge_value);
        test.setMaxISupply_charge(iSupply_charge_value);
        iSupply_charge_edit.setText(QString::number(iSupply_charge_value));

        const double iSupplyWing_noCharge_value = 4;
        saveSettings("iSupplyWing_noCharge_edit", iSupplyWing_noCharge_value);
        test.setMaxISupplyWing_noCharge(iSupplyWing_noCharge_value);
        iSupplyWing_noCharge_edit.setText(QString::number(iSupplyWing_noCharge_value));

        const double iSupplyWing_charge_value = 180;
        saveSettings("iSupplyWing_charge_edit", iSupplyWing_charge_value);
        test.setMaxISupplyWing_charge(iSupplyWing_charge_value);
        iSupplyWing_charge_edit.setText(QString::number(iSupplyWing_charge_value));

        const double wingLoss_value = 0.01;
        saveSettings("wingLoss_edit", wingLoss_value);
        test.setMaxWingLoss(wingLoss_value);
        wingLoss_edit.setText(QString::number(wingLoss_value));

        const double wingChargeCurrent_value = 48;
        saveSettings("wingChargeCurrent_edit", wingChargeCurrent_value);
        test.setminChargeCurrent(wingChargeCurrent_value);
        wingChargeCurrent_edit.setText(QString::number(wingChargeCurrent_value));

        const double packetLoss_value = 0.01;
        saveSettings("packetLoss_edit", packetLoss_value);
        test.setMaxPacketLoss(packetLoss_value);
        packetLoss_edit.setText(QString::number(packetLoss_value));

    });
    settings->addRow(tr("&Bus"), &bus_edit);
    bus_edit.setText(QString("1"));
    bus_edit.setEnabled(false);
    settings->addRow(tr("&Measuring duration"), &measuring_duration);
    measuring_duration.setPlaceholderText(QString("[ms]"));
    connect(&measuring_duration, &QLineEdit::returnPressed, this,
            [=](){
        auto value = measuring_duration.text();
        if (!isNumber(value)) {
            QToolTip::showText(measuring_duration.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 10000.0;
        const double MAX_NUMBER = 60000.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(measuring_duration.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("measuring_duration", number);
        test.setMeasuringDuration(number);
    });
    settings->addRow(tr("&Max iSupply (passive)"), &iSupply_noCharge_edit);
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
        test.setMaxISupply_noCharge(number);
    });
    settings->addRow(tr("&Max iSupply (active)"), &iSupply_charge_edit);
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
        test.setMaxISupply_charge(number);
    });
    settings->addRow(tr("&Max iSupplyWing (passive)"), &iSupplyWing_noCharge_edit);
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
        test.setMaxISupplyWing_noCharge(number);
    });
    settings->addRow(tr("&Max iSupplyWing (active)"), &iSupplyWing_charge_edit);
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
        test.setMaxISupplyWing_charge(number);
    });
    settings->addRow(tr("&Max wing loss"), &wingLoss_edit);
    connect(&wingLoss_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = wingLoss_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(wingLoss_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 0.0;
        const double MAX_NUMBER = 1.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(wingLoss_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("wingLoss_edit", number);
        test.setMaxWingLoss(number);
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
        test.setminChargeCurrent(number);
    });
    settings->addRow(tr("&Max packet loss"), &packetLoss_edit);
    connect(&packetLoss_edit, &QLineEdit::returnPressed, this,
            [=](){
        auto value = packetLoss_edit.text();
        if (!isNumber(value)) {
            QToolTip::showText(packetLoss_edit.mapToGlobal(QPoint(0, 0)), QString("Only digits!"));
            return;
        }
        auto number = value.toDouble();
        const double MIN_NUMBER = 0.0;
        const double MAX_NUMBER = 1.0;
        if (number < MIN_NUMBER || MAX_NUMBER < number) {
            QToolTip::showText(packetLoss_edit.mapToGlobal(QPoint(0, 0)), QString("This field expects a value between %1 and %2").arg(MIN_NUMBER).arg(MAX_NUMBER));
            return;
        }
        saveSettings("packetLoss_edit", number);
        test.setMaxPacketLoss(number);
    });

    auto settings_wrapper = new QWidget(this);
    settings_wrapper->setLayout(settings);

    loadSettings();

    return settings_wrapper;
}

void MainWindow::loadSettings()
{
    QSettings settings("Seatex", "WingSlotTest");
    auto measuring_duration_val = settings.value(QString("measuring_duration"), 10000);
    measuring_duration.setText(measuring_duration_val.toString());
    test.setMeasuringDuration(measuring_duration_val.toInt());

    auto iSupply_noCharge_edit_val = settings.value(QString("iSupply_noCharge_edit"), 8);
    iSupply_noCharge_edit.setText(iSupply_noCharge_edit_val.toString());
    test.setMaxISupply_noCharge(iSupply_noCharge_edit_val.toFloat());

    auto iSupply_charge_edit_val = settings.value(QString("iSupply_charge_edit"), 180);
    iSupply_charge_edit.setText(iSupply_charge_edit_val.toString());
    test.setMaxISupply_charge(iSupply_charge_edit_val.toFloat());

    auto iSupplyWing_noCharge_edit_val = settings.value(QString("iSupplyWing_noCharge_edit"), 4);
    iSupplyWing_noCharge_edit.setText(iSupplyWing_noCharge_edit_val.toString());
    test.setMaxISupplyWing_noCharge(iSupplyWing_noCharge_edit_val.toFloat());

    auto iSupplyWing_charge_edit_val = settings.value(QString("iSupplyWing_charge_edit"), 180);
    iSupplyWing_charge_edit.setText(iSupplyWing_charge_edit_val.toString());
    test.setMaxISupplyWing_charge(iSupplyWing_charge_edit_val.toFloat());

    auto wingLoss_edit_val = settings.value(QString("wingLoss_edit"), 0.01);
    wingLoss_edit.setText(wingLoss_edit_val.toString());
    test.setMaxWingLoss(wingLoss_edit_val.toFloat());

    auto wingChargeCurrent_edit_val = settings.value(QString("wingChargeCurrent_edit"), 48);
    wingChargeCurrent_edit.setText(wingChargeCurrent_edit_val.toString());
    test.setminChargeCurrent(wingChargeCurrent_edit_val.toFloat());

    auto packetLoss_edit_val = settings.value(QString("packetLoss_edit"), 0.01);
    packetLoss_edit.setText(packetLoss_edit_val.toString());
    test.setMaxPacketLoss(packetLoss_edit_val.toFloat());
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

