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
          {QStringLiteral("toolbarSize"), I18N_NOOP("Toolbar"), QStringLiteral("Toolbar"), KIconLoader::Toolbar},
          {QStringLiteral("mainToolbarSize"), I18N_NOOP("Main Toolbar"), QStringLiteral("MainToolbar"), KIconLoader::MainToolbar},
          {QStringLiteral("smallSize"), I18N_NOOP("Small Icons"), QStringLiteral("Small"), KIconLoader::Small},
          {QStringLiteral("panelSize"), I18N_NOOP("Panel"), QStringLiteral("Panel"), KIconLoader::Panel},
          {QStringLiteral("dialogSize"), I18N_NOOP("Dialogs"), QStringLiteral("Dialog"), KIconLoader::Dialog},
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
