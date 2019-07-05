#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "functiontest.h"
#include "widgets/toolframe.h"
#include "widgets/qstatebutton.h"
#include "widgets/qdynamicbar.h"

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

class MainWindow : public ToolFrame
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;


private:
    FunctionTest test;

    //Main widgets
    QLabel test_info;
    QDynamicBar progress_bar;
    QStateButton start_button;
    StatusBitWidget idle_power_status;
    StatusBitWidget active_power_status;
    StatusBitWidget radio_com_status;
    StatusBitWidget packet_loss_status;
    StatusBitWidget charging_status;

    //Settings widgets
    QPushButton reset_button;
    QLineEdit bus_edit;
    QLineEdit measuring_duration;
    QLineEdit iSupply_noCharge_edit;
    QLineEdit iSupply_charge_edit;
    QLineEdit iSupplyWing_noCharge_edit;
    QLineEdit iSupplyWing_charge_edit;
    QLineEdit wingLoss_edit;
    QLineEdit wingChargeCurrent_edit;
    QLineEdit packetLoss_edit;

    QWidget *makeContent();
    void refreshTestUnits();
    void resetDisplay();

    QWidget *makeSettings();
    void loadSettings();
    void saveSettings(const QString &item, QVariant value);
    bool isNumber(const QString &item);
};

#endif // MAINWINDOW_H
