/*
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>
    SPDX-FileCopyrightText: 2016 Olivier Churlaud <olivier@churlaud.com>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "colorsmodel.h"

#include <QCollator>
#include <QDir>
#include <QStandardPaths>

#include <KColorScheme>
#include <KConfigGroup>
#include <KSharedConfig>

#include <algorithm>

#include "colorsapplicator.h"

ColorsModel::ColorsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ColorsModel::~ColorsModel() = default;

int ColorsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_data.count();
}

QVariant ColorsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.count()) {
        return QVariant();
    }

    const auto &item = m_data.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return item.display;
    case SchemeNameRole:
        return item.schemeName;
    case PaletteRole:
        return item.palette;
    case DisabledText:
        return item.palette.color(QPalette::Disabled, QPalette::Text);
    case ActiveTitleBarBackgroundRole:
        return item.activeTitleBarBackground;
    case ActiveTitleBarForegroundRole:
        return item.activeTitleBarForeground;
    case PendingDeletionRole:
        return item.pendingDeletion;
    case RemovableRole:
        return item.removable;
    case AccentActiveTitlebarRole:
        return item.accentActiveTitlebar;
    case Tints:
        return item.tints;
    case TintFactor:
        return item.tintFactor;
    }

    return QVariant();
}

bool ColorsModel::setData(const QModelIndex &index, const QVariant &value, int role)
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

            if (index.row() == selectedSchemeIndex() && pendingDeletion) {
                // move to the next non-pending theme
                const auto nonPending = match(index, PendingDeletionRole, false);
                if (!nonPending.isEmpty()) {
                    setSelectedScheme(nonPending.first().data(SchemeNameRole).toString());
                }
            }

            Q_EMIT pendingDeletionsChanged();
            return true;
        }
    }

    return false;
}

QHash<int, QByteArray> ColorsModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {SchemeNameRole, QByteArrayLiteral("schemeName")},
        {PaletteRole, QByteArrayLiteral("palette")},
        {ActiveTitleBarBackgroundRole, QByteArrayLiteral("activeTitleBarBackground")},
        {ActiveTitleBarForegroundRole, QByteArrayLiteral("activeTitleBarForeground")},
        {DisabledText, QByteArrayLiteral("disabledText")},
        {RemovableRole, QByteArrayLiteral("removable")},
        {AccentActiveTitlebarRole, QByteArrayLiteral("accentActiveTitlebar")},
        {PendingDeletionRole, QByteArrayLiteral("pendingDeletion")},
        {Tints, QByteArrayLiteral("tints")},
        {TintFactor, QByteArrayLiteral("tintFactor")},
    };
}

QString ColorsModel::selectedScheme() const
{
    return m_selectedScheme;
}

void ColorsModel::setSelectedScheme(const QString &scheme)
{
    if (m_selectedScheme == scheme) {
        return;
    }

    m_selectedScheme = scheme;

    Q_EMIT selectedSchemeChanged(scheme);
    Q_EMIT selectedSchemeIndexChanged();
}

int ColorsModel::indexOfScheme(const QString &scheme) const
{
    auto it = std::find_if(m_data.begin(), m_data.end(), [&scheme](const ColorsModelData &item) {
        return item.schemeName == scheme;
    });

    if (it != m_data.end()) {
        return std::distance(m_data.begin(), it);
    }

    return -1;
}

int ColorsModel::selectedSchemeIndex() const
{
    return indexOfScheme(m_selectedScheme);
}

void ColorsModel::load()
{
    beginResetModel();

    const int oldCount = m_data.count();

    m_data.clear();

    QStringList schemeFiles;

    const QStringList schemeDirs =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes"), QStandardPaths::LocateDirectory);
    for (const QString &dir : schemeDirs) {
        const QStringList fileNames = QDir(dir).entryList(QStringList{QStringLiteral("*.colors")});
        for (const QString &file : fileNames) {
            const QString suffixedFileName = QLatin1String("color-schemes/") + file;
            // can't use QSet because of the transform below (passing const QString as this argument discards qualifiers)
            if (!schemeFiles.contains(suffixedFileName)) {
                schemeFiles.append(suffixedFileName);
            }
        }
    }

    std::transform(schemeFiles.begin(), schemeFiles.end(), schemeFiles.begin(), [](const QString &item) {
        return QStandardPaths::locate(QStandardPaths::GenericDataLocation, item);
    });

    for (const QString &schemeFile : schemeFiles) {
        const QFileInfo fi(schemeFile);
        const QString baseName = fi.baseName();

        KSharedConfigPtr config = KSharedConfig::openConfig(schemeFile, KConfig::SimpleConfig);
        KConfigGroup group(config, "General");
        const QString name = group.readEntry("Name", baseName);

        const QPalette palette = KColorScheme::createApplicationPalette(config);

        QColor activeTitleBarBackground, activeTitleBarForeground;
        if (KColorScheme::isColorSetSupported(config, KColorScheme::Header)) {
            KColorScheme headerColorScheme(QPalette::Active, KColorScheme::Header, config);
            activeTitleBarBackground = headerColorScheme.background().color();
            activeTitleBarForeground = headerColorScheme.foreground().color();
        } else {
            KConfigGroup wmConfig(config, QStringLiteral("WM"));
            activeTitleBarBackground = wmConfig.readEntry("activeBackground", palette.color(QPalette::Active, QPalette::Highlight));
            activeTitleBarForeground = wmConfig.readEntry("activeForeground", palette.color(QPalette::Active, QPalette::HighlightedText));
        }

        const bool colorActiveTitleBar = group.readEntry("accentActiveTitlebar", false);

        ColorsModelData item{
            name,
            baseName,
            palette,
            activeTitleBarBackground,
            activeTitleBarForeground,
            fi.isWritable(),
            colorActiveTitleBar,
            false, // pending deletion
            group.hasKey("TintFactor"),
            group.readEntry<qreal>("TintFactor", DefaultTintFactor),
        };

        m_data.append(item);
    }

    // Sort case-insensitively
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_data.begin(), m_data.end(), [&collator](const ColorsModelData &a, const ColorsModelData &b) {
        return collator.compare(a.display, b.display) < 0;
    });

    endResetModel();

    // an item might have been added before the currently selected one
    if (oldCount != m_data.count()) {
        Q_EMIT selectedSchemeIndexChanged();
    }
}

QStringList ColorsModel::pendingDeletions() const
{
    QStringList pendingDeletions;

    for (const auto &item : m_data) {
        if (item.pendingDeletion) {
            pendingDeletions.append(item.schemeName);
        }
    }

    return pendingDeletions;
}

void ColorsModel::removeItemsPendingDeletion()
{
    for (int i = m_data.count() - 1; i >= 0; --i) {
        if (m_data.at(i).pendingDeletion) {
            beginRemoveRows(QModelIndex(), i, i);
            m_data.remove(i);
            endRemoveRows();
        }
    }
}
