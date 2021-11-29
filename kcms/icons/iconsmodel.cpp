/*
    SPDX-FileCopyrightText: 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
    SPDX-FileCopyrightText: 2000 Antonio Larrosa <larrosa@kde.org>
    SPDX-FileCopyrightText: 2000 Geert Jansen <jansen@kde.org>
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    KDE Frameworks 5 port
    SPDX-FileCopyrightText: 2013 Jonathan Riddell <jr@jriddell.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "iconsmodel.h"

#include <QCollator>
#include <QFileIconProvider>

#include <KIconTheme>

#include "iconssettings.h"

IconsModel::IconsModel(IconsSettings *iconsSettings, QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(iconsSettings)
{
}

IconsModel::~IconsModel() = default;

int IconsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_data.count();
}

QVariant IconsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.count()) {
        return QVariant();
    }

    const auto &item = m_data.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return item.display;
    case ThemeNameRole:
        return item.themeName;
    case DescriptionRole:
        return item.description;
    case RemovableRole:
        return item.removable;
    case PendingDeletionRole:
        return item.pendingDeletion;
    }

    return QVariant();
}

bool IconsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_data.count()) {
        return false;
    }

    if (role == PendingDeletionRole) {
        auto &item = m_data[index.row()];

        const bool pendingDeletion = value.toBool();

        if (item.pendingDeletion != pendingDeletion) {
            item.pendingDeletion = pendingDeletion;
            Q_EMIT dataChanged(index, index, {PendingDeletionRole});

            // if we delete current selected theme move to the next non-pending theme
            const auto nonPending = match(index, PendingDeletionRole, false);
            if (m_settings->theme() == index.data(ThemeNameRole) && !nonPending.isEmpty()) {
                m_settings->setTheme(nonPending.first().data(ThemeNameRole).toString());
            }
            Q_EMIT pendingDeletionsChanged();
            return true;
        }
    }

    return false;
}

QHash<int, QByteArray> IconsModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {DescriptionRole, QByteArrayLiteral("description")},
        {ThemeNameRole, QByteArrayLiteral("themeName")},
        {RemovableRole, QByteArrayLiteral("removable")},
        {PendingDeletionRole, QByteArrayLiteral("pendingDeletion")},
    };
}

void IconsModel::load()
{
    beginResetModel();

    m_data.clear();

    const QStringList themes = KIconTheme::list();

    m_data.reserve(themes.count());

    for (const QString &themeName : themes) {
        KIconTheme theme(themeName);
        if (!theme.isValid()) {
            // qCWarning(KCM_ICONS) << "Not a valid theme" << themeName;
        }
        if (theme.isHidden()) {
            continue;
        }

        IconsModelData item{
            theme.name(),
            themeName,
            theme.description(),
            themeName != KIconTheme::defaultThemeName() && QFileInfo(theme.dir()).isWritable(),
            false // pending deletion
        };

        m_data.append(item);
    }

    // Sort case-insensitively
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_data.begin(), m_data.end(), [&collator](const IconsModelData &a, const IconsModelData &b) {
        return collator.compare(a.display, b.display) < 0;
    });

    endResetModel();
}

QStringList IconsModel::pendingDeletions() const
{
    QStringList pendingDeletions;

    for (const auto &item : m_data) {
        if (item.pendingDeletion) {
            pendingDeletions.append(item.themeName);
        }
    }

    return pendingDeletions;
}

void IconsModel::removeItemsPendingDeletion()
{
    for (int i = m_data.count() - 1; i >= 0; --i) {
        if (m_data.at(i).pendingDeletion) {
            beginRemoveRows(QModelIndex(), i, i);
            m_data.remove(i);
            endRemoveRows();
        }
    }
}
