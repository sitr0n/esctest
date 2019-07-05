#include "functiontest.h"
#include <iostream>
#include <unistd.h>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDebug>
#include <QTime>
#include <QMessageBox>
#include <QDateTime>


FunctionTest::FunctionTest(QObject *parent)
    : QObject(parent)
    , svr_(GetBirdcomServer(0, 0))
    , teststate(IDLE)
    , channel(1)
    , activeMeasuringDuration(10000)
    , maxISupply_noCharge(8)
    , maxISupplyWing_noCharge(4)
    , maxISupply_charge(180)
    , maxISupplyWing_charge(180)
    , minChargeCurrent(48)
    , maxWingLoss(0.01)
    , minWingLqi(0.7)
    , minSlotLqi(0.7)
    , maxPacketLoss(0.01)
{
    connect(&timer, SIGNAL(timeout()), this, SLOT(doTest()));
}

void FunctionTest::putTextStream(QTextStream *stream)
{
    output = stream;
}

int FunctionTest::findUnits() try
{
    units = WingSlot::findDevices(channel, svr_);
    for (const auto& unit: units) {
        print(QString("Unit[%1] is ready\n").arg(unit->id()));
    }
    return units.size();
} catch (...) {
    emit error(QString("Esvr is not running."));
    return 0;
}

eBird::Birdcom_var FunctionTest::GetBirdcomServer(int argc, char** argv) {
    eBird::Birdcom_var svr_tmp = 0;
    OrbInitializer orb_init (argc, argv);
    ObjectLocator ol ("Birdcom", "");
    for (int i = 1; i < argc; i++) {
        qDebug() << QString(argv[i]);
        ol.ProcessArgument(argv, i);
    }
    CORBA::ORB_var orbs = orb_init.Init();
    CORBA::Object_var objs = ol.Locate (orbs);
    svr_tmp = eBird::Birdcom::_narrow (objs);
    svr_tmp->GetDate();     // exception if the server is not running
    return svr_tmp;
}

void FunctionTest::start()
{
    if (!WingSlot::connected(svr_)) {
        print(QString("Error: Could not connect to esvr"));
        return;
    }
    if (units.size() < 1) {
        print(QString("Error: No units to test"));
        return;
    }
    results.clear();
    teststate = IDLE;
    timer.start(TICK_RATE);
}

bool FunctionTest::isRunning() const
{
    return (teststate != IDLE);
}

void FunctionTest::stop()
{
    teststate = IDLE;
    timer.stop();
}

QString FunctionTest::getTestVersion() const
{
    return TEST_VERSION;
}

void FunctionTest::setMeasuringDuration(int value)
{
    activeMeasuringDuration = value;
}

void FunctionTest::setMaxISupply_noCharge(float value)
{
    maxISupply_noCharge = value;
}

void FunctionTest::setMaxISupplyWing_noCharge(float value)
{
    maxISupplyWing_noCharge = value;
}

void FunctionTest::setMaxISupply_charge(float value)
{
    maxISupply_charge = value;
}

void FunctionTest::setMaxISupplyWing_charge(float value)
{
    maxISupplyWing_charge = value;
}

void FunctionTest::setminChargeCurrent(float value)
{
    minChargeCurrent = value;
}

void FunctionTest::setMaxWingLoss(float value)
{
    maxWingLoss = value;
}

void FunctionTest::setMinWingLqi(float value)
{
    minWingLqi = value;
}

void FunctionTest::setMinSlotLqi(float value)
{
    minSlotLqi = value;
}

void FunctionTest::setMaxPacketLoss(float value)
{
    maxPacketLoss = value;
}


void FunctionTest::doTest() try
{
    switch (teststate) {
    case IDLE:
        for (const auto& slot: units) {
            slot->setCharge(false);
        }
        teststate = MEASURE_POWER_NOCHARGE;
        stopwatch.start();
        break;

    case MEASURE_POWER_NOCHARGE:
        for (const auto& slot: units) {
            SmartCageData1 data;
            if (slot->getData(data)) {
                results[slot->id()].nocharge.iSupply_samples.push_back(data.iSupply);
                results[slot->id()].nocharge.iSupplyWing_samples.push_back(data.iSupplyWing);
            }
        }
        if (stopwatch.elapsed() > IDLE_MEASURING_DURATION) {
            for (const auto& slot: units) {
                auto first_sample = results[slot->id()].nocharge.iSupply_samples.begin();
                auto last_sample = results[slot->id()].nocharge.iSupply_samples.end();
                auto num_samples = results[slot->id()].nocharge.iSupply_samples.size();
                results[slot->id()].nocharge.iSupply = std::accumulate(first_sample, last_sample, 0.0) / num_samples;
                results[slot->id()].nocharge.iSupply_samples.clear();
            }

            for (const auto& slot: units) {
                auto first_sample = results[slot->id()].nocharge.iSupplyWing_samples.begin();
                auto last_sample = results[slot->id()].nocharge.iSupplyWing_samples.end();
                auto num_samples = results[slot->id()].nocharge.iSupplyWing_samples.size();
                results[slot->id()].nocharge.iSupplyWing = std::accumulate(first_sample, last_sample, 0.0) / num_samples;
                results[slot->id()].nocharge.iSupplyWing_samples.clear();
            }

            bool no_errors = true;
            for (const auto& slot: units) {
                if (results[slot->id()].nocharge.iSupply > maxISupply_noCharge) {
                    print(QString("Unit[#%1] measured iSupply [%2], while expecting less than [%3]")
                          .arg(slot->id()).arg(results[slot->id()].nocharge.iSupply).arg(maxISupply_noCharge));

                    no_errors = false;
                }
                if (results[slot->id()].nocharge.iSupplyWing > maxISupplyWing_noCharge) {
                    print(QString("Unit[#%1] measured iSupplyWing [%2], while expecting less than [%3]")
                          .arg(slot->id()).arg(results[slot->id()].nocharge.iSupplyWing).arg(maxISupplyWing_noCharge));

                    no_errors = false;
                }
            }
            emit idle_power_test(no_errors);

            for (const auto& slot: units) {
                slot->setCharge(true);
            }
            teststate = CONNECT_WINGS;
        }
        break;

    case CONNECT_WINGS:
        //timer.setInterval(TICK_RATE_WINGASSOCIATION);
//        if (connectWings()) {
//            teststate = MEASURE_POWER_CHARGE;
//            stopwatch.restart();
//            //timer.setInterval(TICK_RATE);
//            emit wings_found();
//        }

        for (const auto& slot: units) {
            SmartCageData1 data;
            slot->getData(data);
            if (!data.flags.bWingIdentified) {
                //wings_connected = false;
                slot->connectWing();
            }
        }
        break;

    case MEASURE_POWER_CHARGE:
        for (const auto& slot: units) {
            SmartCageData1 data;
            if (slot->getData(data) && data.flags.bWingIdentified) {
                results[slot->id()].charge.iSupply_samples.push_back(data.iSupply);
                results[slot->id()].charge.iSupplyWing_samples.push_back(data.iSupplyWing);
                results[slot->id()].wing.WingLoss_samples.push_back(data.WingLoss);
                results[slot->id()].wing.WingLqi_samples.push_back(data.WingLqi);
                results[slot->id()].slot.SlotLqi_samples.push_back(data.SlotLqi);
            }

        }
        if (stopwatch.elapsed() > activeMeasuringDuration) {
            // iSupply
            for (const auto& slot: units) {
                auto first_sample = results[slot->id()].charge.iSupply_samples.begin();
                auto last_sample = results[slot->id()].charge.iSupply_samples.end();
                auto num_samples = results[slot->id()].charge.iSupply_samples.size();
                results[slot->id()].charge.iSupply = std::accumulate(first_sample, last_sample, 0.0) / num_samples;
                results[slot->id()].charge.iSupply_samples.clear();
            }
            // iSupplyWing
            for (const auto& slot: units) {
                auto first_sample = results[slot->id()].charge.iSupplyWing_samples.begin();
                auto last_sample = results[slot->id()].charge.iSupplyWing_samples.end();
                auto num_samples = results[slot->id()].charge.iSupplyWing_samples.size();
                results[slot->id()].charge.iSupplyWing = std::accumulate(first_sample, last_sample, 0.0) / num_samples;
                results[slot->id()].charge.iSupplyWing_samples.clear();
            }
            // WingLoss
            for (const auto& slot: units) {
                auto first_sample = results[slot->id()].wing.WingLoss_samples.begin();
                auto last_sample = results[slot->id()].wing.WingLoss_samples.end();
                auto num_samples = results[slot->id()].wing.WingLoss_samples.size();
                results[slot->id()].wing.WingLoss = std::accumulate(first_sample, last_sample, 0.0) / num_samples;
                results[slot->id()].wing.WingLoss_samples.clear();
            }
            // WingLqi
            for (const auto& slot: units) {
                auto first_sample = results[slot->id()].wing.WingLqi_samples.begin();
                auto last_sample = results[slot->id()].wing.WingLqi_samples.end();
                auto num_samples = results[slot->id()].wing.WingLqi_samples.size();
                results[slot->id()].wing.WingLqi = std::accumulate(first_sample, last_sample, 0.0) / num_samples;
                results[slot->id()].wing.WingLqi_samples.clear();
            }
            // SlotLqi
            for (const auto& slot: units) {
                auto first_sample = results[slot->id()].slot.SlotLqi_samples.begin();
                auto last_sample = results[slot->id()].slot.SlotLqi_samples.end();
                auto num_samples = results[slot->id()].slot.SlotLqi_samples.size();
                results[slot->id()].slot.SlotLqi = std::accumulate(first_sample, last_sample, 0.0) / num_samples;
                results[slot->id()].slot.SlotLqi_samples.clear();
            }

            bool no_errors = true;
            for (const auto& slot: units) {
                if (results[slot->id()].charge.iSupply > maxISupply_charge) {
                    results[slot->id()].failed = true;
                    no_errors = false;
                    print(QString("Unit[#%1] measured iSupply [%2], while expecting less than [%3]")
                          .arg(slot->id()).arg(results[slot->id()].charge.iSupply).arg(maxISupply_charge));
                }
                if (results[slot->id()].charge.iSupplyWing > maxISupplyWing_charge) {
                    results[slot->id()].failed = true;
                    no_errors = false;
                    print(QString("Unit[#%1] measured iSupplyWing [%2], while expecting less than [%3]")
                          .arg(slot->id()).arg(results[slot->id()].charge.iSupplyWing).arg(maxISupplyWing_charge));
                }
            }
            emit active_power_test(no_errors);

            teststate = GET_STATISTICS;
        }
        break;

    case GET_STATISTICS:
        if (logResults()) {
            teststate = SAVE_RESULTS;
        }
        break;

    case SAVE_RESULTS:
        if (!WriteToPalm()) {
            print(QString("Error: Couldn't write PALM data to file!"));
        }
        teststate = ts_done;
        break;

    case ts_done:
        stop();
        break;

    default:
        emit error(QString("Test state out of bounds."));
        break;
    }

} catch (...) {
    stop();
    emit error(QString("Lost connection to Esvr."));
}

void FunctionTest::print(const QString &message)
{
    if (output) {
        *output << message;
    } else {
        std::cout << message.toStdString();
    }
}

bool FunctionTest::connectWings()
{
    bool wings_connected = true;
    for (const auto& slot: units) {
        SmartCageData1 data;
        slot->getData(data);
        if (!data.flags.bWingIdentified) {
            wings_connected = false;
            slot->connectWing();
        }
    }
    return wings_connected;
}

bool FunctionTest::testRadio()
{
    bool no_errors = true;
    for (const auto& slot: units) {
        if (results[slot->id()].wing.WingLoss > 0.001) {
            results[slot->id()].failed = true;
            no_errors = false;
        }
        if (results[slot->id()].wing.WingLqi < 0.7) {
            results[slot->id()].failed = true;
            no_errors = false;
        }
    }
    return no_errors;
}

bool FunctionTest::logResults()
{
    bool no_errors = true;

    for (const auto& slot: units) {
        SmartCageData1 data;
        if (!slot->getData(data)) { // SUBOPTIMAL
            return false;
        }
        results[slot->id()].wing.WingSerial = data.WingSerial;
        results[slot->id()].wing.WingBatCapacity = data.WingBatCapacity;
        results[slot->id()].wing.WingBatCurrent = data.WingBatCurrent;
        results[slot->id()].wing.WingBatVolt = data.WingBatVolt;
        results[slot->id()].wing.WingHumidity = data.WingHumidity;
        results[slot->id()].wing.WingTemperature = data.WingTemperature;
        results[slot->id()].slot.SlotTemperature = data.Temperature;

        if (results[slot->id()].wing.WingLoss > maxWingLoss) {
            results[slot->id()].failed = true;
            no_errors = false;
            print(QString("Unit[#%1] measured WingLoss [%2], while expecting less than [%3]\n").arg(slot->id()).arg(results[slot->id()].wing.WingLoss).arg(maxWingLoss));
        }
        if (results[slot->id()].wing.WingLqi < minWingLqi) {
            results[slot->id()].failed = true;
            no_errors = false;
            print(QString("Unit[#%1] measured WingLqi [%2], while expecting more than [%3]\n").arg(slot->id()).arg(results[slot->id()].wing.WingLqi).arg(minWingLqi));
        }
        if (results[slot->id()].slot.SlotLqi < minSlotLqi) {
            results[slot->id()].failed = true;
            no_errors = false;
            print(QString("Unit[#%1] measured SlotLqi [%2], while expecting more than [%3]\n").arg(slot->id()).arg(results[slot->id()].slot.SlotLqi).arg(minSlotLqi));
        }
        emit radio_com_test(no_errors);

        no_errors = true;
        if (data.WingBatCurrent < minChargeCurrent) {
            results[slot->id()].failed = true;
            no_errors = false;
            print(QString("Unit[#%1] measured WingBatCurrent [%2], while expecting more than [%3]\n").arg(slot->id()).arg(data.WingBatCurrent).arg(minChargeCurrent));
        }
        emit charge_test(no_errors);

        no_errors = true;
        if (slot->packetLoss() > maxPacketLoss) {
            results[slot->id()].failed = true;
            no_errors = false;
            print(QString("Unit[#%1] measured PacketLoss [%2], while expecting less than [%3]\n").arg(slot->id()).arg(slot->packetLoss()).arg(maxPacketLoss));
        }
    }
    emit packet_loss_test(no_errors);
    return true;
}


bool FunctionTest::WriteToPalm()
{
    auto path = QString("%1/%2/%3_%4.txt")
            .arg(QString::fromUtf8(std::getenv("HOME")))
            .arg("PALM/log")
            .arg("Activity_Test_118_eBird_Wing")
            .arg(QDateTime::currentDateTime().toString("yyyy_MM_dd"));
    QFile file;
    file.setFileName(path);
    bool newFile = false;
    if (!file.exists()) {
        newFile = true;
    }

    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        return false;
    }
    QTextStream stream(&file);
    if (newFile) {
        stream << QString("Product");
        stream << QString("|%1").arg("SerialNo"); // Slot serial number
        stream << QString("|%1").arg("Timestamp"); // ISO timestamp
        stream << QString("|%1").arg("Import configuration"); // Import configuration
        stream << QString("|%1").arg("Source"); // Source
        stream << QString("|%1").arg("User name"); // Username
        stream << QString("|%1").arg("Comment"); // Comment
        stream << QString("|%1").arg("ProductFamily"); // ProductFamily
        stream << QString("|%1").arg("MACAddress"); // MACAddress
        stream << QString("|%1").arg("FirmwareVersion"); // FirmwareVersion
        stream << QString("|%1").arg("Mode"); // Mode
        stream << QString("|%1").arg("SerialNoCard"); // SerialNoCard
        stream << QString("|%1").arg("HardwareRevision"); // HardwareRevision
        stream << QString("|%1").arg("TestReport"); // TestReport
        stream << QString("|%1").arg("SoftwareVersion"); // SoftwareVersion
        stream << QString("|%1").arg("Free"); // Free

        stream << QString("|%1").arg("TestVersion"); // TestVersion
        stream << QString("|%1").arg("Approved"); // Approved
        stream << QString("|%1").arg("WingSerial"); // WingSerial
        stream << QString("|%1").arg("MeasuringDuration_noCharge"); // No charge measuring duration
        stream << QString("|%1").arg("iSupply_noCharge"); // iSupply no charge
        stream << QString("|%1").arg("iSupplyWing_noCharge"); // iSupplyWing no charge
        stream << QString("|%1").arg("MeasurigDuration_charge"); // Charged measuring duration
        stream << QString("|%1").arg("iSupply_charge"); // iSupply charge
        stream << QString("|%1").arg("iSupplyWing_charge"); // iSupplyWing charge
        stream << QString("|%1").arg("MeasuringDuration_noLoad"); // No load measuring duration -- to be implemented
        stream << QString("|%1").arg("iSupply_noLoad"); // iSupply no load -- to be implemented
        stream << QString("|%1").arg("iSupplyWing_noLoad"); // iSupplyWing no load -- to be implemented
        stream << QString("|%1").arg("WingBatCapacity"); // WingBatCapacity
        stream << QString("|%1").arg("WingBatCurrent"); // WingBatCurrent
        stream << QString("|%1").arg("WingBatVolt"); // WingBatVolt
        stream << QString("|%1").arg("WingHumidity"); // WingHumidity
        stream << QString("|%1").arg("WingLoss"); // WingLoss
        stream << QString("|%1").arg("WingLqi"); // WingLqi
        stream << QString("|%1").arg("WingTemperature"); // WingTemperature
        stream << QString("|%1").arg("SlotLoss"); // SlotLoss
        stream << QString("|%1").arg("SlotLqi"); // SlotLqi
        stream << QString("|%1").arg("SlotTemperature"); // SlotTemperature
        stream << QString("\n");
    }
    for (const auto& slot: units) {
        stream << QString("eB-WCB_001");
        stream << QString("|%1").arg(slot->id()); // Slot serial number
        stream << QString("|%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss")); // ISO timestamp
        stream << QString("|%1").arg("118"); // Import configuration
        stream << QString("|%1").arg(""); // Source
        stream << QString("|%1").arg(""); // Username
        stream << QString("|%1").arg(""); // Comment
        stream << QString("|%1").arg("eBird"); // ProductFamily
        stream << QString("|%1").arg(""); // MACAddressee
        stream << QString("|%1").arg(slot->firmwareVersion()); // FirmwareVersion
        stream << QString("|%1").arg(""); // Mode
        stream << QString("|%1").arg(""); // SerialNoCard
        stream << QString("|%1").arg(""); // HardwareRevision
        stream << QString("|%1").arg(""); // TestReport
        stream << QString("|%1").arg(""); // SoftwareVersion
        stream << QString("|%1").arg(""); // Free

        stream << QString("|%1").arg(getTestVersion()); // TestVersion
        stream << QString("|%1").arg(!results[slot->id()].failed); // Approved (!Failed)
        stream << QString("|%1").arg(results[slot->id()].wing.WingSerial); // WingSerial
        stream << QString("|%1").arg(IDLE_MEASURING_DURATION); // No charge measuring duration
        stream << QString("|%1").arg(results[slot->id()].nocharge.iSupply); // iSupply no charge
        stream << QString("|%1").arg(results[slot->id()].nocharge.iSupplyWing); // iSupplyWing no charge
        stream << QString("|%1").arg(activeMeasuringDuration); // Charged measuring duration
        stream << QString("|%1").arg(results[slot->id()].charge.iSupply); // iSupply charge
        stream << QString("|%1").arg(results[slot->id()].charge.iSupplyWing); // iSupplyWing charge
        stream << QString("|%1").arg(""); // No load measuring duration -- to be implemented
        stream << QString("|%1").arg(""); // iSupply no load -- to be implemented
        stream << QString("|%1").arg(""); // iSupplyWing no load -- to be implemented
        stream << QString("|%1").arg(results[slot->id()].wing.WingBatCapacity); // WingBatCapacity
        stream << QString("|%1").arg(results[slot->id()].wing.WingBatCurrent); // WingBatCurrent
        stream << QString("|%1").arg(results[slot->id()].wing.WingBatVolt); // WingBatVolt
        stream << QString("|%1").arg(results[slot->id()].wing.WingHumidity); // WingHumidity
        stream << QString("|%1").arg(QString::number(results[slot->id()].wing.WingLoss * 100, 'g', 5)); // WingLoss
        stream << QString("|%1").arg(results[slot->id()].wing.WingLqi); // WingLqi
        stream << QString("|%1").arg(results[slot->id()].wing.WingTemperature); // WingTemperature
        stream << QString("|%1").arg(QString::number(results[slot->id()].slot.SlotLoss * 100, 'g', 5)); // SlotLoss
        stream << QString("|%1").arg(results[slot->id()].slot.SlotLqi); // SlotLqi
        stream << QString("|%1").arg(results[slot->id()].slot.SlotTemperature); // SlotTemperature
        stream << QString("\n");
    }

    file.close();
    return true;
}


