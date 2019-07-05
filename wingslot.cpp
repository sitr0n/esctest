#include "wingslot.h"
#include <iostream>
#include <unistd.h>

#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QDir>



WingSlot::WingSlot(int serial, eBird::Birdcom_var &svr, QObject *parent)
    : QObject(parent)
    , serialnumber(serial)
    , packages_sent(0)
    , packages_lost(0)
    , svr_(svr)
{
    //Implement watchdog in case of bad PCB or bad wire
    while (!GetFirmwareVersion()) {}
    while (!SetWingComInterval(WINGCOM_SAMPLE_RATE)) {}
    while (!SetWingAutoAssociation(true)) {}
    while (!AssociateWing()) {}
}

int WingSlot::id() const
{
    return serialnumber;
}

QString WingSlot::firmwareVersion() const
{
    return QString(reinterpret_cast<const char*>(firmware));
}

double WingSlot::packetLoss() const
{
    return static_cast<double>(packages_lost) / packages_sent;
}

std::vector<std::unique_ptr<WingSlot>> WingSlot::findDevices(int bus, eBird::Birdcom_var &svr)
{
    eBird::ScanResult_var devices;
    svr->QueryBirds(bus, MAX_SCAN_DELAY, devices);
    std::vector<std::unique_ptr<WingSlot>> container;
    for (uint i = 0; i < devices->length(); ++i) {
        std::unique_ptr<WingSlot> unit(new WingSlot(devices[i].BirdId, svr));
        container.push_back(std::move(unit));
    }
    return container;
}

bool WingSlot::connected(eBird::Birdcom_var &svr) try
{
    svr->GetDate();
    return true;
} catch (...) {
    return false;
}

bool WingSlot::GetFirmwareVersion()
{
    ++packages_sent;
    eBird::SwupData_var version;
    //eBird::QueryResponse qr;
    /* Note that the Id function is used to also add the bird to the esvr list */
    /* GetFirmware version is the first message and thus the only message required to use Id fuction */
    auto qr = svr_->SwupGetBodyVersionId(1, serialnumber, true, version);
    if (qr == eBird::QrAcknowledged && version->length() == 62) {
        memcpy(firmware, version->get_buffer(), version->length());
        return true;
    }
    ++packages_lost;
    return false;
}

bool WingSlot::SetWingComInterval(float interval)
{
    ++packages_sent;
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len = 0;
    id.length(128);
    int type = EncodeEscmSetWingComInterval(&id[0], &len, interval);
    id.length(len);
    auto qr = svr_->SendSmartCageGeneric(1, serialnumber, type, id, od);
    if (qr == eBird::QrAcknowledged) {
        return true;
    }
    ++packages_lost;
    return false;
}

bool WingSlot::SetWingAutoAssociation(bool bOn)
{
    ++packages_sent;
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len = 0;
    id.length(128);
    int type = EncodeEscmSetAutoAssociation(&id[0], &len, bOn);
    id.length(len);
    auto qr = svr_->SendSmartCageGeneric(1, serialnumber, type, id, od);
    if(qr == eBird::QrAcknowledged){
        return true;
    }
    ++packages_lost;
    return false;
}

bool WingSlot::AssociateWing()
{
    ++packages_sent;
    eBird::QueryResponse qr;
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len =0;
    int type;
    id.length(128);
    type = EncodeEscmAssociateWing(&id[0], &len);
    id.length(len);
    qr = svr_->SendSmartCageGeneric(1, serialnumber, type, id, od);
    if(qr == eBird::QrAcknowledged){
        return true;
    }
    ++packages_lost;
    return false;
}


bool WingSlot::measurePowerUsage()
{
    ++packages_sent;
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    SmartCageData1 d1;
    int len = 0;
    int type;
    id.length(128); // Set to maximum for now.
    type = EncodeEscmGetData1(&id[0], &len);
    id.length(len);
    std::cout << "id length " << len << std::endl;
    auto qr = svr_->SendSmartCageGeneric(1, serialnumber, type, id, od);
    if (qr == eBird::QrAcknowledged) {
        DecodeEscmGetData1(&od[0], &d1);

        if (d1.flags.bWingIdentified) {
            data.WingSerial = d1.WingSerial;
            results.WingBatCurrent = d1.WingBatCurrent;
            data.WingBatCapacity = d1.WingBatCapacity;
            data.WingBatVolt = d1.WingBatVolt;
            data.WingHumidity = d1.WingHumidity;
            data.WingTemperature = d1.WingTemperature;


        }
        std::cout << "wing msg loss " << d1.WingLoss << std::endl;

        std::cout << "wing " << (d1.flags.bWingIdentified ? "Identified" : "not identified") << std::endl;
        std::cout << "iSupply " << d1.iSupply << std::endl;
        std::cout << "iSupplyWing " << d1.iSupplyWing << std::endl;
        std::cout << "iReturnWing " << d1.iReturnWing << std::endl;
        std::cout << "vReturnWing " << d1.vReturnWing << std::endl;
        std::cout << "Temperature " << d1.Temperature << std::endl;
        std::cout << "wing serial " << d1.WingSerial << std::endl;
        std::cout << "SlotLqi " << d1.SlotLqi << std::endl;
        std::cout << "WingLqi " << d1.WingLqi << std::endl;
        std::cout << "WingBatCapacity " << d1.WingBatCapacity << std::endl;
        std::cout << "WingBatCurrent " << d1.WingBatCurrent << std::endl;
        std::cout << "WingHumidity " << d1.WingHumidity << std::endl;
        std::cout << "WingTemperature " << d1.WingTemperature << std::endl;
        std::cout << "WingBatVolt " << d1.WingBatVolt << std::endl;
        std::cout << "WingAngleMes " << d1.WingAngleMes << std::endl;
        //EncodeEscmEnablePower()

        return true;
    }

    ++packages_lost;
    return false;
}


bool WingSlot::getData(SmartCageData1 &data)
{
    ++packages_sent;
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len = 0;
    int type;
    id.length(128); // Set to maximum for now.
    type = EncodeEscmGetData1(&id[0], &len);
    id.length(len);
    auto qr = svr_->SendSmartCageGeneric(1, serialnumber, type, id, od);
    if (qr == eBird::QrAcknowledged){
        DecodeEscmGetData1(&od[0], &data);
        return true;
    }
    ++packages_lost;
    return false;
}

void WingSlot::connectWing()
{
    AssociateWing();
}

bool WingSlot::setCharge(bool enable)
{
    ++packages_sent;
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len = 0;
    id.length(128);
    int type = EncodeEscmEnablePower(&id[0], &len, enable);
    id.length(len);
    auto qr = svr_->SendSmartCageGeneric(1, serialnumber, type, id, od);
    if (qr == eBird::QrAcknowledged) {
        return true;
    }
    ++packages_lost;
    return false;
}
