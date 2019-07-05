#ifndef QSTATEBUTTON_H
#define QSTATEBUTTON_H

#include <QWidget>
#include <map>
#include <vector>
#include <memory>
#include <QIcon>
#include <QPushButton>

class QStateButton : public QPushButton
{
    Q_OBJECT
public:
    explicit QStateButton(QWidget *parent = nullptr);
    void addState(const QString &name, QIcon *icon = nullptr);
    void setState(const QString &name);
    virtual void setMinimumSize(const QSize &size);

signals:
    void state_change(const QString &state);

private slots:
    void mousePressEvent(QMouseEvent *e);

private:
    int currentState;
    std::vector<QString> states;
    std::map<QString, std::unique_ptr<QIcon>> icons;

    void updateButton();
    void clearButton();
};

#endif // QSTATEBUTTON_H
