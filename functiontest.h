#ifndef FUNCTIONTEST_H
#define FUNCTIONTEST_H

#include <QObject>
#include <QTextStream>
#include <QTimer>
#include <QTime>
#include "wingslot.h"
#include <vector>
#include <map>

#include "ebird.hh"
#include "get_svrobj.h"
#include "orb_init.h"

typedef enum {
    IDLE,
    MEASURE_POWER_NOCHARGE,
    CONNECT_WINGS,
    MEASURE_POWER_CHARGE,
    GET_STATISTICS,
    SAVE_RESULTS,
    ts_done
} t_test_state;



class FunctionTest : public QObject
{
    Q_OBJECT
public:
    explicit FunctionTest(QObject *parent = nullptr);
    void putTextStream(QTextStream *stream);
    int findUnits();
    void start();
    bool isRunning() const;
    void stop();

    QString getTestVersion() const;

    void setMeasuringDuration(int value);
    void setMaxISupply_noCharge(float value);
    void setMaxISupplyWing_noCharge(float value);
    void setMaxISupply_charge(float value);
    void setMaxISupplyWing_charge(float value);
    void setminChargeCurrent(float value);
    void setMaxWingLoss(float value);
    void setMinWingLqi(float value);
    void setMinSlotLqi(float value);
    void setMaxPacketLoss(float value);

signals:
    void idle_power_test(bool approved);
    void wings_found();
    void active_power_test(bool approved);
    void radio_com_test(bool approved);
    void charge_test(bool approved);
    void packet_loss_test(bool approved);

    void finished(std::vector<int> results); // what was the plan here? test summary?
    void error(const QString &message);


private slots:
    void doTest();

private:
    void print(const QString &message);
    bool connectWings();
    bool testRadio();
    bool logResults();
    bool WriteToPalm();
    eBird::Birdcom_var GetBirdcomServer(int argc, char **argv);
    eBird::Birdcom_var svr_;
    QTimer timer;
    QTime stopwatch;
    QTime association_stopwatch;
    t_test_state teststate;
    int channel;
    std::vector<std::unique_ptr<WingSlot>> units;
    QTextStream *output;

    int activeMeasuringDuration;
    float maxISupply_noCharge;
    float maxISupplyWing_noCharge;
    float maxISupply_charge;
    float maxISupplyWing_charge;
    float minChargeCurrent;
    float maxWingLoss;
    float minWingLqi;
    float minSlotLqi;
    float maxPacketLoss;

    const QString TEST_VERSION = "1.02.00";
    const int TICK_RATE = 1000;
    const int WING_ASSOCIATION_INTERVAL = 11000;
    const int TICK_RATE_WINGASSOCIATION = 5000;
    const int IDLE_MEASURING_DURATION = 10000;
    const int ACTIVE_MEASURING_DURATION = 20000;
    const float WINGCOM_SAMPLE_RATE = 0.1;
    const double MAX_SCAN_DELAY = 1;



    struct PowerData {
        float iSupply;
        float iSupplyWing;
        std::vector<float> iSupply_samples;
        std::vector<float> iSupplyWing_samples;
    };

    struct WingData {
        unsigned long WingSerial;
        float WingBatCapacity;
        float WingBatVolt;
        float WingBatCurrent;
        float WingHumidity;
        float WingTemperature;

        float WingLqi;
        std::vector<float> WingLqi_samples;

        float WingLoss;
        std::vector<float> WingLoss_samples;
    };

    struct SlotData {
        float SlotTemperature;

        float SlotLoss;
        std::vector<float> SlotLoss_samples;

        float SlotLqi;
        std::vector<float> SlotLqi_samples;

    };

    struct TestData {
        PowerData nocharge;
        PowerData charge;
        WingData wing;
        SlotData slot;
        bool failed;
    };
    std::map<int, TestData> results;
};

#endif // FUNCTIONTEST_H



