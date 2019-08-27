#include "wingchargeblockmanager.h"
#include "wingslot.h"
#include "escfunctest.h"

WingChargeBlockManager::WingChargeBlockManager(QObject *parent)
    : QThread(parent)
    , m_quit(false)
{
    WingSlot::findCommunicationServer(0, 0);
}

WingChargeBlockManager::~WingChargeBlockManager()
{
    m_quit = true;
}

void WingChargeBlockManager::scan(WingChargeBlockManager::Channels channels)
{
    QMutexLocker locker(&m_mutex);
    m_channels = channels;
    if (!isRunning()) {
        start();
    } else {
        m_cond.wakeOne();
    }
}

void WingChargeBlockManager::view(WingChargeBlockManager::WingChargeBlock unit)
{
    QMutexLocker locker(&m_mutex); // timeout locker here
    m_viewUnit = unit.id;
    if (!isRunning()) {
        start();
    } else {
        m_cond.wakeOne();
    }
}

void WingChargeBlockManager::test(WingChargeBlockManager::WingChargeBlock unit)
{
    QMutexLocker locker(&m_mutex);
    m_testUnit = unit.id;
    if (!isRunning()) {
        start();
    } else {
        m_cond.wakeOne();
    }
}

void WingChargeBlockManager::record(WingChargeBlockManager::WingChargeBlocks units)
{
    QMutexLocker locker(&m_mutex);
    m_recordUnits = units;
    if (!isRunning()) {
        start();
    } else {
        m_cond.wakeOne();
    }
}

void WingChargeBlockManager::run()
{
    WingSlot::SlotList units;
    EscFuncTest *test = nullptr;

    m_mutex.lock();
    auto channels = m_channels;
    //auto viewUnit = m_viewUnit;
    auto testUnit = m_testUnit;
    auto recordUnits = m_recordUnits;
    clearInput();
    m_mutex.unlock();

    while (!m_quit) {
        if (channels.size() > 0) {
            std::sort(channels.begin(), channels.end());
            channels.erase(std::unique(channels.begin(), channels.end()), channels.end());
            units.clear();
            for (const auto& channel: channels) {
                auto new_units = WingSlot::discoverUnits(channel);
                units.insert(units.end(), std::make_move_iterator(new_units.begin()), std::make_move_iterator(new_units.end()));
            }
            units = WingSlot::sort(units);
        }

        if (testUnit && !test->isRunning()) {
            if (test != nullptr) {
                delete test;
            }
            // Awaiting change to WingSlot
            for (WingSlot& unit: units) {
                if (unit.id() == testUnit) {
                    test = new EscFuncTest(unit);
                    test->start(unit.sampling());
                    //viewUnit = testUnit;
                    // How to emit view with correct interval.. hmmm

//                    connect(test, &EscFuncTest::finished, this,
//                            [=](const EscFuncTest::State &state, const bool &passed, const QString &message){
//                        switch (state) {
//                        case EscFuncTest::State::PASSIVE :
//                            emit testFinished(state, passed, EscFuncTest::getDuration().pairing);
//                            break;
//                        case EscFuncTest::State::PAIRING :
//                            emit testFinished(state, passed, EscFuncTest::getDuration().active);
//                            break;
//                        case EscFuncTest::State::ACTIVE :
//                            emit testFinished(state, passed);
//                            break;
//                        }
//                        if (!message.isEmpty()) {
//                            emit output(message);
//                        }

//                    });
                }
            }
        }

        if (recordUnits.size() > 0) {

        }



        m_mutex.lock();
        m_cond.wait(&m_mutex);

        clearInput();
        m_mutex.unlock();
    }
}

void WingChargeBlockManager::clearInput()
{
    m_channels.clear();
    m_viewUnit = 0;
    m_testUnit = 0;
    m_recordUnits.clear();
}

