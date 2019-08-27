#pragma once

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <vector>

class WingChargeBlockManager : public QThread
{
    Q_OBJECT
public:
    explicit WingChargeBlockManager(QObject *parent = nullptr);
    ~WingChargeBlockManager() override;
    struct WingChargeBlock
    {
        int id;
        QString firmware;
        bool approved;
        bool recording;
        // const bool &present; ?
    };
    struct Data {
        int id;
        float inputCurrent;
        float outputCurrent;
        float temperature;
        float LQI;
        float loss;
        bool charging;
        bool paired;
        struct {
            int id;
            float temperature;
            float LQI;
            float loss;
        } wing;
    };
    typedef std::vector<WingChargeBlock> WingChargeBlocks;
    typedef std::vector<int> Channels;

    void scan(Channels channels);
    void view(WingChargeBlock unit);
    void test(WingChargeBlock unit);
    void record(WingChargeBlocks units);
    void run() override;

signals:
    void scanFinished(const WingChargeBlocks units);
    void currentUnit(const Data data, bool mutableUnit);
    void testFinished(int testStage, bool approved, int estimatedWaitTime = 0);

    void output(const QString &message);
    void error(const QString &message);

protected:
    void clearInput();


private:
    Channels m_channels;
    int m_viewUnit;
    int m_testUnit;
    WingChargeBlocks m_recordUnits;



    QMutex m_mutex;
    QWaitCondition m_cond;
    bool m_quit;
};
