#include "palm.h"

#include <QDateTime>
#include <QRegExp>

PALM::PALM(const QString &title)
    : m_title(title)
{
    setPath("~");
    m_header.push_back(QString("Product"));
    m_header.push_back(QString("SerialNo"));
    m_header.push_back(QString("Timestamp"));
    m_header.push_back(QString("Import configuration"));
    m_header.push_back(QString("Source"));
    m_header.push_back(QString("User name"));
    m_header.push_back(QString("Comment"));
    m_header.push_back(QString("ProductFamily"));
    m_header.push_back(QString("MACAddress"));
    m_header.push_back(QString("FirmwareVersion"));
    m_header.push_back(QString("Mode"));
    m_header.push_back(QString("SerialNoCard"));
    m_header.push_back(QString("HardwareRevision"));
    m_header.push_back(QString("TestReport"));
    m_header.push_back(QString("SoftwareVersion"));
    m_header.push_back(QString("Free"));
}

void PALM::setTitle(const QString &title)
{
    m_title = title;
}

void PALM::setPath(const QString &path)
{
    m_path = (path.at(path.size() - 1) == '/') ? path : path + '/';
    if (m_path.at(0) == '~') {
        m_path.replace(0, 1, QString::fromUtf8(std::getenv("HOME")));
    }
}

void PALM::addColumn(const std::vector<QString> &header)
{
    m_header.insert(m_header.end(), header.begin(), header.end());
    m_customData.insert(m_customData.end(), header.begin(), header.end());
}

void PALM::addColumn(const QString &section)
{
    m_header.push_back(section);
    m_customData.push_back(section);
}

bool PALM::setValue(const QString &item, const QVariant &v)
{
    for (const auto& field: m_header) {
        if (QString::compare(field, item) == 0) {
            m_data[item] = QVariant::fromValue(v);
            return true;
        }
    }
    return false;
}

bool PALM::save()
{
    FileResponse status;
    QTextStream stream(openFile(status));

    if (status == FileResponse::ERROR) {
        return false;
    }
    if (status == FileResponse::NEW) {
        writeHeader(stream);
    }
    writeData(stream);
    clearData();
    stream.device()->close();
    delete stream.device();
    return true;
}

QFile *PALM::openFile(PALM::FileResponse &status)
{
    auto file = new QFile;
    auto filename = QString("%1_%2.txt").arg(m_title).arg(QDateTime::currentDateTime().toString("yyyy_MM_dd"));
    file->setFileName(m_path + filename);

    if (!file->exists()) {
        status = FileResponse::NEW;
    } else {
        status = FileResponse::OPEN;
    }
    if (!file->open(QIODevice::Append | QIODevice::Text)) {
        status = FileResponse::ERROR;
    }
    return file;
}

void PALM::writeHeader(QTextStream &file)
{
    for (const auto& field: m_header) {
        file << field << ((field == m_header.at(m_header.size() - 1)) ? '\n' : '|');
    }
}

void PALM::writeData(QTextStream &file)
{
    setValue("Timestamp", QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss"));
    for (const auto& field: m_header) {
        switch (m_data[field].type()) {
        case QMetaType::Bool :
            file << QString::number(m_data[field].toBool());
            break;

        case QMetaType::Double :
            file << QString::number(m_data[field].toDouble(), 'f', FORMAT_DECIMALS).remove(QRegExp("\\.?0+$"));
            break;

        case QMetaType::Float :
            file << QString::number(m_data[field].toFloat(), 'f', FORMAT_DECIMALS).remove(QRegExp("\\.?0+$"));
            break;

        default:
            file << m_data[field].toString();
        }
        file << ((field == m_header.at(m_header.size() - 1)) ? '\n' : '|');
    }
}

void PALM::clearData()
{
    for (const auto& field: m_customData) {
        m_data.erase(field);
    }
}
