/*
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>
    SPDX-FileCopyrightText: 2016 Olivier Churlaud <olivier@churlaud.com>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "themesmodel.h"

#include <QCollator>
#include <QDir>
#include <QStandardPaths>

#include <KColorScheme>
#include <KDesktopFile>

#include <KConfigGroup>
#include <KSharedConfig>

#include <algorithm>

ThemesModel::ThemesModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ThemesModel::~ThemesModel() = default;

int ThemesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_data.count();
}

QVariant ThemesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.count()) {
        return QVariant();
    }

    const auto &item = m_data.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return item.display;
    case PluginNameRole:
        return item.pluginName;
    case DescriptionRole:
        return item.description;
    case ColorTypeRole:
        return item.type;
    case IsLocalRole:
        return item.isLocal;
    case PendingDeletionRole:
        return item.pendingDeletion;
    }
    return QVariant();
}

bool ThemesModel::setData(const QModelIndex &index, const QVariant &value, int role)
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

            if (index.row() == selectedThemeIndex() && pendingDeletion) {
                // move to the next non-pending theme
                const auto nonPending = match(index, PendingDeletionRole, false);
                if (!nonPending.isEmpty()) {
                    setSelectedTheme(nonPending.first().data(PluginNameRole).toString());
                }
            }

            Q_EMIT pendingDeletionsChanged();
            return true;
        }
    }

    return false;
}

QHash<int, QByteArray> ThemesModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {PluginNameRole, QByteArrayLiteral("pluginName")},
        {DescriptionRole, QByteArrayLiteral("description")},
        {ColorTypeRole, QByteArrayLiteral("colorType")},
        {IsLocalRole, QByteArrayLiteral("isLocal")},
        {PendingDeletionRole, QByteArrayLiteral("pendingDeletion")},
    };
}

QString ThemesModel::selectedTheme() const
{
    return m_selectedTheme;
}

void ThemesModel::setSelectedTheme(const QString &pluginName)
{
    if (m_selectedTheme == pluginName) {
        return;
    }

    m_selectedTheme = pluginName;

    Q_EMIT selectedThemeChanged(pluginName);

    Q_EMIT selectedThemeIndexChanged();
}

int ThemesModel::pluginIndex(const QString &pluginName) const
{
    const auto results = match(index(0, 0), PluginNameRole, pluginName, 1, Qt::MatchExactly);
    if (results.count() == 1) {
        return results.first().row();
    }

    return -1;
}

int ThemesModel::selectedThemeIndex() const
{
    return pluginIndex(m_selectedTheme);
}

void ThemesModel::load()
{
    beginResetModel();

    const int oldCount = m_data.count();

    m_data.clear();

    // Get all desktop themes
    QStringList themes;
    const QStringList packs =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("plasma/desktoptheme"), QStandardPaths::LocateDirectory);
    for (const QString &ppath : packs) {
        const QDir cd(ppath);
        const QStringList &entries = cd.entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
        for (const QString &pack : entries) {
            const QString _metadata = ppath + QLatin1Char('/') + pack + QStringLiteral("/metadata.desktop");
            if (QFile::exists(_metadata)) {
                themes << _metadata;
            }
        }
    }

    for (const QString &theme : qAsConst(themes)) {
        int themeSepIndex = theme.lastIndexOf(QLatin1Char('/'), -1);
        const QString themeRoot = theme.left(themeSepIndex);
        int themeNameSepIndex = themeRoot.lastIndexOf(QLatin1Char('/'), -1);
        const QString packageName = themeRoot.right(themeRoot.length() - themeNameSepIndex - 1);

        KDesktopFile df(theme);

        if (df.noDisplay()) {
            continue;
        }

        QString name = df.readName();
        if (name.isEmpty()) {
            name = packageName;
        }
        const bool isLocal = QFileInfo(theme).isWritable();
        bool hasPluginName = std::any_of(m_data.begin(), m_data.end(), [&](const ThemesModelData &item) {
            return item.pluginName == packageName;
        });
        if (!hasPluginName) {
            // Plasma Theme creates a KColorScheme out of the "color" file and falls back to system colors if there is none
            const QString colorsPath = themeRoot + QLatin1String("/colors");
            const bool followsSystemColors = !QFileInfo::exists(colorsPath);
            ColorType type = FollowsColorTheme;
            if (!followsSystemColors) {
                const KSharedConfig::Ptr config = KSharedConfig::openConfig(colorsPath);
                const QPalette palette = KColorScheme::createApplicationPalette(config);
                const int windowBackgroundGray = qGray(palette.window().color().rgb());
                if (windowBackgroundGray < 192) {
                    type = DarkTheme;
                } else {
                    type = LightTheme;
                }
            }
            ThemesModelData item{name, packageName, df.readComment(), type, isLocal, false};
            m_data.append(item);
        }
    }

    // Sort case-insensitively
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_data.begin(), m_data.end(), [&collator](const ThemesModelData &a, const ThemesModelData &b) {
        return collator.compare(a.display, b.display) < 0;
    });

    endResetModel();

    // an item might have been added before the currently selected one
    if (oldCount != m_data.count()) {
        Q_EMIT selectedThemeIndexChanged();
    }
}

QStringList ThemesModel::pendingDeletions() const
{
    QStringList pendingDeletions;

    for (const auto &item : qAsConst(m_data)) {
        if (item.pendingDeletion) {
            pendingDeletions.append(item.pluginName);
        }
    }

    return pendingDeletions;
}

void ThemesModel::removeRow(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_data.erase(m_data.begin() + row);
    endRemoveRows();
}
