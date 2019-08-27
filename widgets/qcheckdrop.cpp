#include "qcheckdrop.h"
#include <QDebug>
QCheckDrop::QCheckDrop(QWidget *parent)
    : QComboBox(parent)
{
    setModel(new QStandardItemModel(0, 1, this));
    setMinimumWidth(160);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

//    connect(this, &QComboBox::activated, this,
//            [=](int index){
//        qDebug() << QString("Combo[%1]").arg(index);
//    });

    connect(this, SIGNAL(activated(int)), this, SLOT(toggleItem(int)));
}

void QCheckDrop::putHeader(const QString &title)
{
    auto header = new QStandardItem(title);
    qobject_cast<QStandardItemModel*>(model())->setItem(0, 0, header);
}

void QCheckDrop::addRow(int id, const QString &name)
{
    auto view = qobject_cast<QStandardItemModel*>(model());
    auto item = newRow(name);
    view->appendRow(item);
    setModel(view);

    m_checkBoxes[id] = item;
    m_identifiers.push_back(id);
}

void QCheckDrop::toggleRows()
{
    bool changed = false;
    for (const auto& row: m_checkBoxes) {
        if (row.second->checkState() == Qt::Unchecked) {
            row.second->setCheckState(Qt::Checked);
            changed = true;
        }
    }
    if (changed) {
        return;
    }
    for (const auto& row: m_checkBoxes) {
        row.second->setCheckState(Qt::Unchecked);
    }
}

std::vector<int> QCheckDrop::checkedItems()
{
    std::vector<int> container;
    for (const auto& row : m_checkBoxes) {
        if (row.second->checkState() == Qt::Checked) {
            container.push_back(row.first);
        }
    }
    return container;
}

std::vector<int> QCheckDrop::uncheckedItems()
{
    std::vector<int> container;
    for (const auto& row : m_checkBoxes) {
        if (row.second->checkState() == Qt::Unchecked) {
            container.push_back(row.first);
        }
    }
    return container;
}

void QCheckDrop::clear()
{
    while (model()->rowCount() > 1) {
        model()->removeRow(1);
    }
    m_identifiers.clear();
    m_checkBoxes.clear();
}

void QCheckDrop::toggleItem(int /* index */)
{
    toggleRows();
}

QStandardItem *QCheckDrop::newRow(const QString &name)
{
    auto item = new QStandardItem(name); // Save away in list for clean up once clear() is called?
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    return item;
}
