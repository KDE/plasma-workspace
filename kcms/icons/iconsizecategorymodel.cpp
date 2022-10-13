/*
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "iconsizecategorymodel.h"
#include <KIconLoader>
#include <KLocalizedString>

IconSizeCategoryModel::IconSizeCategoryModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_data({
          {QStringLiteral("mainToolbarSize"), i18n("Main Toolbar"), QStringLiteral("MainToolbar"), KIconLoader::MainToolbar},
          {QStringLiteral("toolbarSize"), i18n("Secondary Toolbars"), QStringLiteral("Toolbar"), KIconLoader::Toolbar},
          {QStringLiteral("smallSize"), i18n("Small Icons"), QStringLiteral("Small"), KIconLoader::Small},
          {QStringLiteral("dialogSize"), i18n("Dialogs"), QStringLiteral("Dialog"), KIconLoader::Dialog},
      })
{
}

IconSizeCategoryModel::~IconSizeCategoryModel() = default;

int IconSizeCategoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_data.count();
}

QVariant IconSizeCategoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.count()) {
        return QVariant();
    }

    const auto &item = m_data.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return item.display;
    case ConfigKeyRole:
        return item.configKey;
    case ConfigSectionRole:
        return item.configSection;
    case KIconLoaderGroupRole:
        return item.kIconloaderGroup;
    }

    return QVariant();
}

QHash<int, QByteArray> IconSizeCategoryModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
    roleNames[ConfigKeyRole] = QByteArrayLiteral("configKey");
    roleNames[ConfigSectionRole] = QByteArrayLiteral("configSectionRole");
    roleNames[KIconLoaderGroupRole] = QByteArrayLiteral("KIconLoaderGroup");
    return roleNames;
}
