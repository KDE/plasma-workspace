/***************************************************************************
 *   Copyright (C) 2019 Konrad Materka <materka@gmail.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "sortedsystemtraymodel.h"
#include "systemtraymodel.h"
#include "debug.h"

#include <QList>

static const QList<QString> s_categoryOrder = {QStringLiteral("UnknownCategory"),
                                               QStringLiteral("ApplicationStatus"),
                                               QStringLiteral("Communications"),
                                               QStringLiteral("SystemServices"),
                                               QStringLiteral("Hardware")};

SortedSystemTrayModel::SortedSystemTrayModel(SortingType sorting, QObject *parent)
    : QSortFilterProxyModel(parent),
      m_sorting(sorting)
{
    setSortLocaleAware(true);
    sort(0);
}

bool SortedSystemTrayModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (m_sorting) {
    case SortedSystemTrayModel::SortingType::ConfigurationPage:
        return lessThanConfigurationPage(left, right);
    case SortedSystemTrayModel::SortingType::SystemTray:
        return lessThanSystemTray(left, right);
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

bool SortedSystemTrayModel::lessThanConfigurationPage(const QModelIndex &left, const QModelIndex &right) const
{
    const int categoriesComparison = compareCategoriesAlphabetically(left, right);
    if (categoriesComparison == 0) {
        return QSortFilterProxyModel::lessThan(left, right);
    } else {
        return categoriesComparison < 0;
    }
}

bool SortedSystemTrayModel::lessThanSystemTray(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant itemId = sourceModel()->data(left, static_cast<int>(BaseModel::BaseRole::ItemId));
    if (itemId.isValid() && itemId.toString() == QLatin1String("org.kde.plasma.notifications")) {
        return true;
    }

    const int categoriesComparison = compareCategoriesOrderly(left, right);
    if (categoriesComparison == 0) {
        return QSortFilterProxyModel::lessThan(left, right);
    } else {
        return categoriesComparison < 0;
    }
}

int SortedSystemTrayModel::compareCategoriesAlphabetically(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant leftData = sourceModel()->data(left, static_cast<int>(BaseModel::BaseRole::Category));
    QString leftCategory = leftData.isNull() ? QStringLiteral("UnknownCategory") : leftData.toString();

    QVariant rightData = sourceModel()->data(right, static_cast<int>(BaseModel::BaseRole::Category));
    QString rightCategory = rightData.isNull() ? QStringLiteral("UnknownCategory") : rightData.toString();

    return QString::localeAwareCompare(leftCategory, rightCategory);
}

int SortedSystemTrayModel::compareCategoriesOrderly(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant leftData = sourceModel()->data(left, static_cast<int>(BaseModel::BaseRole::Category));
    QString leftCategory = leftData.isNull() ? QStringLiteral("UnknownCategory") : leftData.toString();

    QVariant rightData = sourceModel()->data(right, static_cast<int>(BaseModel::BaseRole::Category));
    QString rightCategory = rightData.isNull() ? QStringLiteral("UnknownCategory") : rightData.toString();

    int leftIndex = s_categoryOrder.indexOf(leftCategory);
    if (leftIndex == -1) {
        leftIndex = s_categoryOrder.indexOf(QStringLiteral("UnknownCategory"));
    }

    int rightIndex = s_categoryOrder.indexOf(rightCategory);
    if (rightIndex == -1) {
        rightIndex = s_categoryOrder.indexOf(QStringLiteral("UnknownCategory"));
    }

    return leftIndex - rightIndex;
}
