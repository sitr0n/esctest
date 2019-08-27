#ifndef ESCFUNCTEST_H
#define ESCFUNCTEST_H

#include <QObject>
#include <QTimer>
#include <QTime>
#include <vector>
#include "wingslot.h"
#include "palm.h"


class EscFuncTest : public QObject
{
    Q_OBJECT
public:
    explicit EscFuncTest(WingSlot &unit, QObject *parent = nullptr);
    void start(const int interval);
    void stop();
    bool isRunning() const;

    enum State {
        NONE,
        PASSIVE,
        PAIRING,
        ACTIVE,
        DONE,
    };
    struct Limits {
        float iSupply_passive;
        float iSupplyWing_passive;
        float iSupply_active;
        float iSupplyWing_active;
        float chargeCurent;
        float wingLoss;
        float wingLQI;
        float slotLQI;
        float slotLoss;
    };
    static void setLimits(const Limits &limits);
    static Limits getLimits();
    struct Durations {
        int passive;
        int pairing;
        int active;
    };
    static void setDuration(Durations duration);
    static Durations getDuration();
    static const QString VERSION;

signals:
    void finished(const State &state, const bool &passed, const QString &message = QString(""));
    void error(const QString &message);

protected:
    void sampleData();
    bool evaluate();
    bool approveRadio();
    bool approveCharging();
    bool approvePower();

private:
    WingSlot &m_unit;
    State m_state;
    static Limits s_limit;
    static Durations s_duration;
    PALM m_log;
    bool m_OK;
    QString m_feedback;

    QTimer m_ticker;
    QTime m_passiveWatch;
    QTime m_pairingWatch;
    QTime m_activeWatch;

    std::vector<float> m_iSupplySamples;
    std::vector<float> m_iSupplyWingSamples;
    std::vector<float> m_wingLossSamples;
    std::vector<float> m_wingLQISamples;
    std::vector<float> m_slotLQISamples;

    static const int INERTIA_DELAY = 5000;
};

#endif // ESCFUNCTEST_H
