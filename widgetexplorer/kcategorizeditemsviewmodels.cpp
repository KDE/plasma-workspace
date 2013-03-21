/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "kcategorizeditemsviewmodels_p.h"
#include <klocale.h>

#define COLUMN_COUNT 4

namespace KCategorizedItemsViewModels {

// AbstractItem

QString AbstractItem::name() const
{
    return text();
}

QString AbstractItem::id() const
{
    QString plugin = data().toMap()["pluginName"].toString();

    if (plugin.isEmpty()) {
        return name();
    }

    return plugin;
}

QString AbstractItem::description() const
{
    return "";
}

bool AbstractItem::isFavorite() const
{
    return passesFiltering(Filter("favorite", true));
}

int AbstractItem::running() const
{
    return 0;
}

bool AbstractItem::matches(const QString &pattern) const
{
    return
        name().contains(pattern, Qt::CaseInsensitive) ||
        description().contains(pattern, Qt::CaseInsensitive);
}

// DefaultFilterModel

DefaultFilterModel::DefaultFilterModel(QObject *parent) :
    QStandardItemModel(0, 1, parent)
{
    setHeaderData(1, Qt::Horizontal, i18n("Filters"));
    //This is to make QML that is understand it
    QHash<int, QByteArray> newRoleNames = roleNames();
    newRoleNames[FilterTypeRole] = "filterType";
    newRoleNames[FilterDataRole] = "filterData";
    newRoleNames[SeparatorRole] = "separator";

    setRoleNames(newRoleNames);
    connect(this, SIGNAL(modelReset()),
            this, SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsInserted(QModelIndex, int, int)),
            this, SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsRemoved(QModelIndex, int, int)),
            this, SIGNAL(countChanged()));
}

void DefaultFilterModel::addFilter(const QString &caption, const Filter &filter, const KIcon &icon)
{
    QList<QStandardItem *> newRow;
    QStandardItem *item = new QStandardItem(caption);
    item->setData(qVariantFromValue<Filter>(filter));
    if (!icon.isNull()) {
        item->setIcon(icon);
    }
    item->setData(filter.first, FilterTypeRole);
    item->setData(filter.second, FilterDataRole);

    newRow << item;
    appendRow(newRow);
}

void DefaultFilterModel::addSeparator(const QString &caption)
{
    QList<QStandardItem *> newRow;
    QStandardItem *item = new QStandardItem(caption);
    item->setEnabled(false);
    item->setData(true, SeparatorRole);

    newRow << item;
    appendRow(newRow);
}

QVariantHash DefaultFilterModel::get(int row) const
{
    QModelIndex idx = index(row, 0);
    QVariantHash hash;

    QHash<int, QByteArray>::const_iterator i;
    for (i = roleNames().constBegin(); i != roleNames().constEnd(); ++i) {
        hash[i.value()] = data(idx, i.key());
    }

    return hash;
}

// DefaultItemFilterProxyModel

DefaultItemFilterProxyModel::DefaultItemFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void DefaultItemFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(sourceModel);

    if (!model) {
        kWarning() << "Expecting a QStandardItemModel!";
        return;
    }

    setRoleNames(sourceModel->roleNames());

    QSortFilterProxyModel::setSourceModel(model);
    connect(this, SIGNAL(modelReset()),
            this, SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsInserted(QModelIndex, int, int)),
            this, SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsRemoved(QModelIndex, int, int)),
            this, SIGNAL(countChanged()));
}

QAbstractItemModel *DefaultItemFilterProxyModel::sourceModel() const
{
    return QSortFilterProxyModel::sourceModel();
}

int DefaultItemFilterProxyModel::columnCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return COLUMN_COUNT;
}

QVariant DefaultItemFilterProxyModel::data(const QModelIndex &index, int role) const
{
    return QSortFilterProxyModel::data(index, role);
}

bool DefaultItemFilterProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
    QStandardItemModel *model = (QStandardItemModel *) sourceModel();

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    AbstractItem *item = (AbstractItem *) model->itemFromIndex(index);
    //kDebug() << "ITEM " << (item ? "IS NOT " : "IS") << " NULL\n";

    return item &&
        (m_filter.first.isEmpty() || item->passesFiltering(m_filter)) &&
        (m_searchPattern.isEmpty() || item->matches(m_searchPattern));
}

QVariantHash DefaultItemFilterProxyModel::get(int row) const
{
    QModelIndex idx = index(row, 0);
    QVariantHash hash;

    QHash<int, QByteArray>::const_iterator i;
    for (i = roleNames().constBegin(); i != roleNames().constEnd(); ++i) {
        hash[i.value()] = data(idx, i.key());
    }

    return hash;
}

bool DefaultItemFilterProxyModel::lessThan(const QModelIndex &left,
        const QModelIndex &right) const
{
    return
        sourceModel()->data(left).toString().localeAwareCompare(
            sourceModel()->data(right).toString()) < 0;
}

void DefaultItemFilterProxyModel::setSearchTerm(const QString &pattern)
{
    m_searchPattern = pattern;
    invalidateFilter();
    emit searchTermChanged(pattern);
}

QString DefaultItemFilterProxyModel::searchTerm() const
{
    return m_searchPattern;
}

void DefaultItemFilterProxyModel::setFilter(const Filter &filter)
{
    m_filter = filter;
    invalidateFilter();
    emit filterChanged();
}

void DefaultItemFilterProxyModel::setFilterType(const QString type)
{
    m_filter.first = type;
    invalidateFilter();
    emit filterChanged();
}

QString DefaultItemFilterProxyModel::filterType() const
{
    return m_filter.first;
}

void DefaultItemFilterProxyModel::setFilterQuery(const QVariant query)
{
    m_filter.second = query;
    invalidateFilter();
    emit filterChanged();
}

QVariant DefaultItemFilterProxyModel::filterQuery() const
{
    return m_filter.second;
}

}
