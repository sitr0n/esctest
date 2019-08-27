#include "qcheckview.h"

QCheckView::QCheckView(QWidget *parent)
    : QListView(parent)
{
    setModel(new QStandardItemModel(0, 0, this));
    setMinimumWidth(160);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    connect(this, &QListView::pressed, this,
            [=](const QModelIndex &index){
        emit selected(m_identifiers.at(index.row()));
    });
}

void QCheckView::addRow(int id, const QString &name)
{
    auto view = qobject_cast<QStandardItemModel*>(model());
    auto item = newRow(name);
    view->appendRow(item);
    setModel(view);

    m_checkBoxes[id] = item;
    m_identifiers.push_back(id);
}


void QCheckView::toggleRows()
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

std::vector<int> QCheckView::checkedItems()
{
    std::vector<int> container;
    for (const auto& row : m_checkBoxes) {
        if (row.second->checkState() == Qt::Checked) {
            container.push_back(row.first);
        }
    }
    return container;
}

void QCheckView::clear()
{
    model()->removeRows(0, model()->rowCount());
    m_identifiers.clear();
    m_checkBoxes.clear();
}

QStandardItem *QCheckView::newRow(const QString &name)
{
    auto item = new QStandardItem(name); // Save away in list for clean up once clear() is called?
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    return item;
}

