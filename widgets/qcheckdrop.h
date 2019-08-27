#pragma once

#include <QComboBox>
#include <QCheckBox>
#include <QStandardItem>

#include <vector>
#include <queue>

class QCheckDrop : public QComboBox
{
    Q_OBJECT
public:
    explicit QCheckDrop(QWidget *parent = nullptr);
    void putHeader(const QString &title);
    void addRow(int id, const QString &name);
    void toggleRows();
    std::vector<int> checkedItems();
    std::vector<int> uncheckedItems();
    void clear();

    //virtual void showPopup() override;

signals:
    void selected(const int id);

protected slots:
    void toggleItem(int);

protected:
    QStandardItem* newRow(const QString &name);

private:
    std::map<int, QStandardItem*> m_checkBoxes;
    std::vector<int> m_identifiers;

};
