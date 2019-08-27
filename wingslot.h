#ifndef WINGSLOT_H
#define WINGSLOT_H

#include <QObject>
#include <QTimer>
#include <QTime>
#include "ebird.hh"
#include "smartcageif.h"
#include <vector>
#include <functional>
#include <memory>

class WingSlot : public QObject
{
    Q_OBJECT
public:
    struct WingData {
        bool dataPresent = false;
        int serial;
        float humidity;
        float temperature;
        float batCapacity;
        float batVolt;
        float batCurrent;
        float iReturn;
        float vReturn;
        float loss;
        float LQI;
    };

    struct Stats {
        QString firmware;
        double iSupply;
        double iSupplyWing;
        double loss;
        double LQI;
        double temperature;
        unsigned long dataAge;
        WingData wing;
    };
    typedef std::reference_wrapper<WingSlot> Unit;
    typedef std::vector<Unit> SlotList;
    static bool findCommunicationServer(int argc, char **argv);
    static void setProcessingLoad(const double loadFactor);
    static SlotList discoverUnits(int bus);
    static void tuneSampling(const double &loadFactor);
    static SlotList sort(SlotList &list);

    int id() const;
    const Stats& stats() const;
    bool isPaired() const;
    void pair();
    bool setAutoPair(bool enable);
    bool setCharge(bool enable);
    bool isCharging() const;
    bool setSampling(int interval);
    int sampling() const;

signals:
    void new_data(const Stats& stats);
    void error(const QString &message);

protected:
    WingSlot(int id, int bus);
    static std::vector<Unit> getReferences();
    template <template <typename, typename> class Container, typename Value, typename Allocator = std::allocator<Value>>
    static auto getReferences(const Container<Value, Allocator> &container) -> Container<std::reference_wrapper<typename Value::element_type>, Allocator>;
    static bool getFirmware(int id, int bus, QString &firmware);
    static void removeOldUnits();

    bool isOutdated() const;
    int inactivityDuration() const;
    void registerActivity(const bool presence);
    void setFirmware(const QString &firmware);
    bool getData(SmartCageData1 &data);
    bool setWingSampling(float interval);
    bool startPairing();
    bool processResponse(bool ok);

private:
    int m_id;
    int m_bus;
    QTimer m_ticker;
    QTime m_pairingWatch;
    bool m_pairing;
    bool m_charging;
    Stats m_data;
    QTime m_inactivityWatch;
    QTime m_freezeWatch;

    static double s_loadFactor;
    static eBird::Birdcom_var s_esvr;
    static std::vector<std::unique_ptr<WingSlot>> s_slots;

    static const int WINGDATA_PER_SAMPLE = 10;          // [UNUSED]
    static const int PAIRING_TIMEOUT = 11000;           // [Milliseconds]
    static const int ACTIVITY_TIMEOUT = 500;            // [Milliseconds]
    static constexpr double WING_COM_INTERVAL = 0.1;    // [Seconds]
    static constexpr double MAX_SCAN_DELAY = 1.0;       // [Seconds]
};

#endif // WINGSLOT_H
