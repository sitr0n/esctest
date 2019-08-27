#ifndef PALM_H
#define PALM_H

#include <QObject>
#include <vector>
#include <map>
#include <memory>
#include <QFile>
#include <QTextStream>

class PALM : public QObject
{
    Q_OBJECT
public:
    PALM(const QString &title = QString("Untitled"));
    void setTitle(const QString &title);
    void setPath(const QString &path);
    void addColumn(const std::vector<QString> &header);
    void addColumn(const QString &section);
    bool setValue(const QString &item, const QVariant &v);
    bool save();

protected:
    enum class FileResponse {
        ERROR,
        OPEN,
        NEW,
    };
    QFile* openFile(FileResponse &status);
    void writeHeader(QTextStream &file);
    void writeData(QTextStream &file);
    void clearData();

private:
    QString m_title;
    QString m_path;
    std::vector<QString> m_header;
    std::vector<QString> m_customData;
    std::map<QString, QVariant> m_data;

    const int FORMAT_DECIMALS = 5;
};

#endif // PALM_H
