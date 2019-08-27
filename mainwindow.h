#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "widgets/toolframe.h"
#include "widgets/qcheckdrop.h"
#include "widgets/qcheckview.h"
#include "widgets/qstatebutton.h"
#include "widgets/qdynamicbar.h"

#include "wingslot.h"
#include "escfunctest.h"
#include "escrecorder.h"
#include "escmonitor.h"

#include <QPushButton>

class MainWindow : public ToolFrame
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

protected:
    void startTest();
    void resetTest();

    QWidget *makeContent();
    QWidget *makeBusScanner();
    QWidget *makeUnitViewer();
    QWidget *makeRecordPanel();
    QWidget *makeTestPanel();

    QWidget *makeSettings();
    void loadSettings();
    void saveSettings(const QString &item, QVariant value);
    bool isNumber(const QString &item);
    void findCommunicationServer(int argc, char** argv);

private:
    WingSlot::SlotList m_units;
    std::queue<WingSlot*> m_test_queue;

    EscFuncTest *m_test = nullptr;
    EscRecorder *m_recorder = nullptr;

    //Main widgets
    QWidget m_content;
    QWidget m_busScanner;
    QWidget m_unitEditor;
    QWidget m_recordPanel;

    QWidget m_testPanel;
    QStateButton m_testButton;
    QDynamicBar m_progressBar;
    StatusBitWidget *m_testPassiveLED;
    StatusBitWidget *m_testPairingLED;
    StatusBitWidget *m_testActiveLED;
    EscMonitor m_unitBrowser;
    QCheckView m_unitViewer;
    QPushButton *m_toggleButton;


    //Settings widgets
    QPushButton reset_button;
    QLineEdit scan_edit;
    QLineEdit passive_duration_edit;
    QLineEdit pairing_duration_edit;
    QLineEdit active_duration_edit;
    QLineEdit iSupply_noCharge_edit;
    QLineEdit iSupply_charge_edit;
    QLineEdit iSupplyWing_noCharge_edit;
    QLineEdit iSupplyWing_charge_edit;
    QLineEdit wingLoss_edit;
    QLineEdit wingChargeCurrent_edit;
    QLineEdit packetLoss_edit;

    const int NUM_BUSSES = 7;
    const double LOAD_FACTOR = 0.05; // default 0.005 ?
    const int LED_SIZE = 25;
    const int TESTING_INTERVAL = 1000;
    const int TESTING_DELAY = 1500;
    const int RECORDING_INTERVAL = 60000;

    const int TEST_DEFAULT_PASSIVE_DURATION = 15000;
    const int TEST_DEFAULT_PAIRING_DURATION = 30000;
    const int TEST_DEFAULT_ACTIVE_DURATION = 30000;
    const double iSupply_noCharge_value = 8;
    const double iSupply_charge_value = 180;
    const double iSupplyWing_noCharge_value = 4;
    const double iSupplyWing_charge_value = 180;
    const double wingLoss_value = 1;
    const double wingChargeCurrent_value = 48;
    const double packetLoss_value = 1;
};

#endif // MAINWINDOW_H
