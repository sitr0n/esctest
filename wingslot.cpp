#include "wingslot.h"
#include "get_svrobj.h"
#include "orb_init.h"
#include <QRegExp>

#include <QDebug>

double WingSlot::s_loadFactor = 0.1;
eBird::Birdcom_var WingSlot::s_esvr;
std::vector<std::unique_ptr<WingSlot>> WingSlot::s_slots;
WingSlot::WingSlot(int id, int bus)
    : m_id(id)
    , m_bus(bus)
    , m_pairing(false)
    , m_charging(false)
{
    QObject::connect(&m_ticker, &QTimer::timeout, this,
            [=](){
        m_freezeWatch.restart();
        SmartCageData1 data;
        if (getData(data)) {
            registerActivity(m_data.iSupply != data.iSupply);                           // - Will kick a watchdog while the data seems volatile
            m_data.iSupply = data.iSupply;                                              // [mA]
            m_data.iSupplyWing = data.iSupplyWing;                                      // [mA]
            m_data.LQI = data.SlotLqi;                                                  // [Percentage]
            m_data.temperature = data.Temperature;                                      // [Celcius]
            m_charging = data.flags.bPowerEnable;                                       // Inductive power supply enabled
            m_data.dataAge = data.DataAge;                                              // Time since data update [ms?]

            if ((m_data.wing.dataPresent = data.flags.bWingDataPresent)) {
                m_data.wing.loss = data.WingLoss * 100;                                 // [Percentage]
                m_data.wing.LQI = data.WingLqi;                                         // [Percentage]
                m_data.wing.serial = data.WingSerial;                                   // [Serial number]
                m_data.wing.humidity = data.WingHumidity;                               // [Percentage]
                m_data.wing.temperature = data.WingTemperature;                         // [Celcius]
                m_data.wing.batCapacity = data.WingBatCapacity;                         // []
                m_data.wing.batVolt = data.WingBatVolt;                                 // [V]
                m_data.wing.batCurrent = data.WingBatCurrent;                           // [mA]
                m_data.wing.iReturn = data.iReturnWing;                                 // [mA]
                m_data.wing.vReturn = data.vReturnWing;                                 // [V]
            }
        }
        emit new_data(m_data);
    });
}

std::vector<WingSlot::Unit> WingSlot::getReferences()
{
    std::vector<Unit> container;
    container.reserve(s_slots.size());
    for (const auto& ptr: s_slots) {
        container.push_back(std::ref(*ptr));
    }
    return container;
}

bool WingSlot::getFirmware(int id, int bus, QString &firmware)
{
    const int response_bytes = 62;
    eBird::SwupData_var version;
    /* Note that the Id function is used to also add the bird to the esvr list */
    /* GetFirmware version is the first message and thus the only message required to use Id fuction */
    auto qr = s_esvr->SwupGetBodyVersionId(bus, id, true, version);
    if (qr == eBird::QrAcknowledged && version->length() == response_bytes) {
        unsigned char data[response_bytes];
        memcpy(data, version->get_buffer(), version->length());
        firmware = QString(reinterpret_cast<const char*>(data));
        return true;
    }
    return false;
}

void WingSlot::removeOldUnits()
{
    int watchCounter = 0;
    while (true) {
        auto oldUnit = std::find_if(s_slots.begin(), s_slots.end(), [&](std::unique_ptr<WingSlot> &unit){
            return unit->isOutdated();
        });
        if (oldUnit != s_slots.end()) {
            qDebug() << QString("Removed one");
            s_slots.erase(std::remove(s_slots.begin(), s_slots.end(), *oldUnit));
        } else {
            break;
        }
        if (++watchCounter > 10000) {
            qDebug() << QString("func[removeOldUnits()] got stuck with a slot list of size [%0]").arg(s_slots.size());
            break;
        }
    }
}

bool WingSlot::isOutdated() const
{
    return (inactivityDuration() > ACTIVITY_TIMEOUT);
}

int WingSlot::inactivityDuration() const
{
    return (m_inactivityWatch.elapsed() - m_freezeWatch.elapsed());
}

void WingSlot::registerActivity(const bool presence)
{
    if (presence) {
        m_inactivityWatch.restart();
    }
}

int WingSlot::id() const
{
    return m_id;
}

const WingSlot::Stats &WingSlot::stats() const
{
    return m_data;
}

void WingSlot::setProcessingLoad(const double loadFactor)
{
    if (loadFactor > 0.0) {
        s_loadFactor = loadFactor;
        if (s_slots.size() > 0) {
            tuneSampling(loadFactor);
        }
    }
}

bool WingSlot::findCommunicationServer(int argc, char** argv) try
{
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
    s_esvr = svr_tmp;
    return true;
} catch(...) {
    return false;
}

WingSlot::SlotList WingSlot::discoverUnits(int bus) try
{
    removeOldUnits();
    eBird::ScanResult_var devices;
    s_esvr->QueryBirds(bus, MAX_SCAN_DELAY, devices);
    for (uint i = 0; i < devices->length(); ++i) {
        auto duplicate = std::find_if(s_slots.begin(), s_slots.end(), [&](std::unique_ptr<WingSlot> &unit){
            return unit->id() == devices[i].BirdId;
        });
        if (duplicate != s_slots.end()) {
            continue;
        }
        QString firmware;
        if (getFirmware(devices[i].BirdId, bus, firmware)) {
            std::unique_ptr<WingSlot> unit(new WingSlot(devices[i].BirdId, bus));
            unit->setFirmware(firmware);
            s_slots.push_back((std::move(unit)));
        }
    }
    tuneSampling(s_loadFactor);
    return getReferences();
} catch (...) {
    return SlotList();
}

void WingSlot::tuneSampling(const double &loadFactor)
{
    auto interval = (int) s_slots.size() / loadFactor;
    for (auto& unit: s_slots) {
        unit->setSampling(interval);
    }
}

bool WingSlot::setWingSampling(float interval) try
{
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len = 0;
    id.length(128);
    int type = EncodeEscmSetWingComInterval(&id[0], &len, interval);
    id.length(len);
    auto qr = s_esvr->SendSmartCageGeneric(m_bus, m_id, type, id, od);
    return processResponse(qr == eBird::QrAcknowledged);
} catch (...) {
    return processResponse(false);
}

bool WingSlot::setAutoPair(bool enable) try
{
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len = 0;
    id.length(128);
    int type = EncodeEscmSetAutoAssociation(&id[0], &len, enable);
    id.length(len);
    auto qr = s_esvr->SendSmartCageGeneric(m_bus, m_id, type, id, od);
    return processResponse(qr == eBird::QrAcknowledged);
} catch (...) {
    return processResponse(false);
}

bool WingSlot::startPairing() try
{
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len = 0;
    int type;
    id.length(128);
    type = EncodeEscmAssociateWing(&id[0], &len);
    id.length(len);
    auto qr = s_esvr->SendSmartCageGeneric(m_bus, m_id, type, id, od);
    return processResponse(qr == eBird::QrAcknowledged);
} catch (...) {
    return processResponse(false);
}

bool WingSlot::processResponse(bool ok)
{
    m_data.loss += static_cast<double> ((ok ? 0.0 : 1.0) - m_data.loss) / 20.0;
    return ok;
}

bool WingSlot::getData(SmartCageData1 &data) try
{
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len = 0;
    int type;
    id.length(128); // Set to maximum for now.
    type = EncodeEscmGetData1(&id[0], &len);
    id.length(len);
    auto qr = s_esvr->SendSmartCageGeneric(m_bus, m_id, type, id, od);
    DecodeEscmGetData1(&od[0], &data);
    return processResponse(qr == eBird::QrAcknowledged);
} catch (...) {
    return processResponse(false);
}

bool WingSlot::isPaired() const
{
    return m_data.wing.dataPresent;
}

void WingSlot::pair()
{
    if (m_pairingWatch.elapsed() > PAIRING_TIMEOUT) {
        m_pairing = false;
    }
    if (!m_pairing && startPairing()) {
        m_pairingWatch.start();
        m_pairing = true;
    }
}

bool WingSlot::setSampling(int interval)
{
    if (interval <= 0) {
        m_ticker.stop();
        return true;
    }
    if (m_ticker.interval() != interval || !m_ticker.isActive()) {
        if (!setWingSampling(WING_COM_INTERVAL)) { // setWingSampling(static_cast<double>(interval/(1000.0 * WINGDATA_PER_SAMPLE)))
            qDebug() << QString("[#%1] did not set wing samp").arg(m_id);
        }

        if (m_ticker.isActive()) {
            m_ticker.setInterval(interval);
        } else {
            m_ticker.start(interval);
        }
        m_inactivityWatch.start();
        m_freezeWatch.start();
    }
    return true;
}

int WingSlot::sampling() const
{
    return (m_ticker.isActive()) ? m_ticker.interval() : 0;
}

void WingSlot::setFirmware(const QString &firmware)
{
    auto str_list = firmware.split(' ');
    QRegExp version_number("(\\d{1,5}\\.\\d{1,5}\\.\\d{1,5})");
    QString stripped_firmware;
    bool section_found = false;
    for (const auto& section: str_list) {
        if (section.contains(version_number)) {
            section_found = true;
        }
        if (section_found) {
            if (!stripped_firmware.isEmpty()) {
                stripped_firmware.append(' ');
            }
            stripped_firmware.append(section);
        }
    }
    m_data.firmware = stripped_firmware;
}

WingSlot::SlotList WingSlot::sort(SlotList &list) // Find a way to sort without losing the memory address of existing elements
{
    std::vector<std::tuple<int, int>> units;
    for (const WingSlot& slot: list) {
        auto tuple = std::make_tuple(slot.id(), slot.m_bus);
        units.push_back(tuple);
    }
    std::sort(units.begin(), units.end());
    units.erase(std::unique(units.begin(), units.end()), units.end());
    SlotList container;
    for (const auto& data: units) {
        QString firmware;
        if (getFirmware(std::get<0>(data), std::get<1>(data), firmware)) {
            std::unique_ptr<WingSlot> slot(new WingSlot(std::get<0>(data), std::get<1>(data)));
            slot->setFirmware(firmware);
            //container.push_back(std::move(slot));
        }
    }
    return container;
}

bool WingSlot::setCharge(bool enable) try
{
    eBird::SmartCageData id;
    eBird::SmartCageData_var od;
    int len = 0;
    id.length(128);
    int type = EncodeEscmEnablePower(&id[0], &len, enable);
    id.length(len);
    auto qr = s_esvr->SendSmartCageGeneric(m_bus, m_id, type, id, od);
    return processResponse(qr == eBird::QrAcknowledged);
} catch (...) {
    return processResponse(false);
}

bool WingSlot::isCharging() const
{
    return m_charging;
}

template <template <typename, typename> class Container, typename Value, typename Allocator = std::allocator<Value>>
Container<std::reference_wrapper<typename Value::element_type>, Allocator> WingSlot::getReferences(const Container<Value, Allocator> &container)
{
    Container<std::reference_wrapper<typename Value::element_type>, Allocator> refs;
    std::transform(container.begin(), container.end(), std::back_inserter(refs),
                   [](typename Value::element_type &x){
            return std::ref(*x);
    });
    return refs;
}
