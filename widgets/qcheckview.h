#pragma once

#include <QListView>
#include <QCheckBox>
#include <QStandardItem>

#include <vector>
#include <queue>

class QCheckView : public QListView
{
    Q_OBJECT
public:
    explicit QCheckView(QWidget *parent = nullptr);
    void addRow(int id, const QString &name);
    void toggleRows();
    std::vector<int> checkedItems();
    void clear();

signals:
    void selected(const int id);

protected:
    QStandardItem* newRow(const QString &name);

private:
    std::map<int, QStandardItem*> m_checkBoxes;
    std::vector<int> m_identifiers;

};
