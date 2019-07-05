#ifndef WINGSLOT_H
#define WINGSLOT_H

#include <QObject>
#include <QTimer>
#include "ebird.hh"
#include "smartcageif.h"
#include <vector>
#include <memory>


class WingSlot : public QObject
{
    Q_OBJECT
public:
    explicit WingSlot(int serial, eBird::Birdcom_var &svr, QObject *parent = nullptr);
    int id() const;
    QString firmwareVersion() const;
    double packetLoss() const;
    bool setCharge(bool enable);
    bool getData(SmartCageData1 &data);
    void connectWing();

    static std::vector<std::unique_ptr<WingSlot>> findDevices(int bus, eBird::Birdcom_var &svr);
    static bool connected(eBird::Birdcom_var &svr);

signals:
    void error(const QString);

private:
    int serialnumber;
    unsigned char firmware[62];
    int packages_sent;
    int packages_lost;

    bool GetFirmwareVersion();
    bool SetWingComInterval(float interval = 0.1);
    bool SetWingAutoAssociation(bool bOn);
    bool AssociateWing();
    bool measurePowerUsage();
    eBird::Birdcom_var &svr_;

    const float WINGCOM_SAMPLE_RATE = 0.1;
    static constexpr double MAX_SCAN_DELAY = 1;

    struct {
        float iSupply;
        float iSupplyWing;
        float iReturnWing;
        float vReturnWing;

        float SlotLossAVERAGE;
        float SlotLossMAX;
        float SlotLossMIN;

        float WingLossAVERAGE;
        float WingLossMAX;
        float WingLossMIN;

        float SlotLqiAVERAGE;
        float SlotLqiMAX;
        float SlotLqiMIN;

        float WingLqiAVERAGE;
        float WingLqiMAX;
        float WingLqiMIN;

        float WingBatCurrent;
    } results;

    struct {
        float SlotTemperature;
        unsigned long WingSerial;
        float WingBatCapacity;
        float WingBatVolt;
        float WingHumidity;
        float WingTemperature;
    } data;
};

#endif // WINGSLOT_H
