/*
    SPDX-FileCopyrightText: 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcategorizeditemsviewmodels_p.h"
#include <KLocalizedString>
#include <QDebug>

#define COLUMN_COUNT 4

namespace KCategorizedItemsViewModels
{
// AbstractItem

QString AbstractItem::name() const
{
    return text();
}

QString AbstractItem::id() const
{
    QString plugin = data().toMap()[QStringLiteral("pluginName")].toString();

    if (plugin.isEmpty()) {
        return name();
    }

    return plugin;
}

QString AbstractItem::description() const
{
    return QLatin1String("");
}

bool AbstractItem::isFavorite() const
{
    return passesFiltering(Filter(QStringLiteral("favorite"), true));
}

int AbstractItem::running() const
{
    return 0;
}

bool AbstractItem::matches(const QString &pattern) const
{
    if (name().contains(pattern, Qt::CaseInsensitive) || description().contains(pattern, Qt::CaseInsensitive)) {
        return true;
    }
    const QStringList itemKeywords = keywords();
    return std::any_of(itemKeywords.begin(), itemKeywords.end(), [&pattern](const QString &keyword) {
        return keyword.startsWith(pattern, Qt::CaseInsensitive);
    });
}

// DefaultFilterModel

DefaultFilterModel::DefaultFilterModel(QObject *parent)
    : QStandardItemModel(0, 1, parent)
{
    setHeaderData(1, Qt::Horizontal, i18n("Filters"));

    connect(this, &QAbstractItemModel::modelReset, this, &DefaultFilterModel::countChanged);
    connect(this, &QAbstractItemModel::rowsInserted, this, &DefaultFilterModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &DefaultFilterModel::countChanged);
}

QHash<int, QByteArray> DefaultFilterModel::roleNames() const
{
    static QHash<int, QByteArray> newRoleNames;
    if (newRoleNames.isEmpty()) {
        newRoleNames = QAbstractItemModel::roleNames();
        newRoleNames[FilterTypeRole] = "filterType";
        newRoleNames[FilterDataRole] = "filterData";
        newRoleNames[SeparatorRole] = "separator";
    }
    return newRoleNames;
}

void DefaultFilterModel::addFilter(const QString &caption, const Filter &filter, const QIcon &icon)
{
    QList<QStandardItem *> newRow;
    QStandardItem *item = new QStandardItem(caption);
    item->setData(QVariant::fromValue<Filter>(filter));
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

    const QHash<int, QByteArray> roles = roleNames();
    for (QHash<int, QByteArray>::const_iterator i = roles.constBegin(); i != roles.constEnd(); ++i) {
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
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(sourceModel);

    if (!model) {
        qWarning() << "Expecting a QStandardItemModel!";
        return;
    }

    QSortFilterProxyModel::setSourceModel(model);
    connect(this, &QAbstractItemModel::modelReset, this, &DefaultItemFilterProxyModel::countChanged);
    connect(this, &QAbstractItemModel::rowsInserted, this, &DefaultItemFilterProxyModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &DefaultItemFilterProxyModel::countChanged);
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

bool DefaultItemFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QStandardItemModel *model = (QStandardItemModel *)sourceModel();

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    AbstractItem *item = (AbstractItem *)model->itemFromIndex(index);
    // qDebug() << "ITEM " << (item ? "IS NOT " : "IS") << " NULL\n";

    return item && (m_filter.first.isEmpty() || item->passesFiltering(m_filter)) && (m_searchPattern.isEmpty() || item->matches(m_searchPattern));
}

QVariantHash DefaultItemFilterProxyModel::get(int row) const
{
    QModelIndex idx = index(row, 0);
    QVariantHash hash;

    const QHash<int, QByteArray> roles = roleNames();
    for (QHash<int, QByteArray>::const_iterator i = roles.constBegin(); i != roles.constEnd(); ++i) {
        hash[i.value()] = data(idx, i.key());
    }

    return hash;
}

bool DefaultItemFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    return sourceModel()->data(left).toString().localeAwareCompare(sourceModel()->data(right).toString()) < 0;
}

void DefaultItemFilterProxyModel::setSearchTerm(const QString &pattern)
{
    m_searchPattern = pattern;
    invalidateFilter();
    Q_EMIT searchTermChanged(pattern);
}

QString DefaultItemFilterProxyModel::searchTerm() const
{
    return m_searchPattern;
}

void DefaultItemFilterProxyModel::setFilter(const Filter &filter)
{
    m_filter = filter;
    invalidateFilter();
    Q_EMIT filterChanged();
}

void DefaultItemFilterProxyModel::setFilterType(const QString &type)
{
    m_filter.first = type;
    invalidateFilter();
    Q_EMIT filterChanged();
}

QString DefaultItemFilterProxyModel::filterType() const
{
    return m_filter.first;
}

void DefaultItemFilterProxyModel::setFilterQuery(const QVariant &query)
{
    m_filter.second = query;
    invalidateFilter();
    Q_EMIT filterChanged();
}

QVariant DefaultItemFilterProxyModel::filterQuery() const
{
    return m_filter.second;
}

}
