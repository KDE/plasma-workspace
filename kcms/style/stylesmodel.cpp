/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "stylesmodel.h"

#include <QCollator>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>

#include <KConfig>
#include <KConfigGroup>

#include <algorithm>

StylesModel::StylesModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

StylesModel::~StylesModel() = default;

int StylesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_data.count();
}

QVariant StylesModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index)) {
        return QVariant();
    }

    const auto &item = m_data.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        if (!item.display.isEmpty()) {
            return item.display;
        }
        return item.styleName;
    case StyleNameRole:
        return item.styleName;
    case DescriptionRole:
        return item.description;
    case ConfigurableRole:
        return !item.configPage.isEmpty();
    }

    return QVariant();
}

QHash<int, QByteArray> StylesModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {StyleNameRole, QByteArrayLiteral("styleName")},
        {DescriptionRole, QByteArrayLiteral("description")},
        {ConfigurableRole, QByteArrayLiteral("configurable")},
    };
}

QString StylesModel::selectedStyle() const
{
    return m_selectedStyle;
}

void StylesModel::setSelectedStyle(const QString &style)
{
    if (m_selectedStyle == style) {
        return;
    }

    const bool firstTime = m_selectedStyle.isNull();
    m_selectedStyle = style;

    if (!firstTime) {
        Q_EMIT selectedStyleChanged(style);
    }
    Q_EMIT selectedStyleIndexChanged();
}

int StylesModel::indexOfStyle(const QString &style) const
{
    auto it = std::find_if(m_data.begin(), m_data.end(), [&style](const StylesModelData &item) {
        return item.styleName == style;
    });

    if (it != m_data.end()) {
        return std::distance(m_data.begin(), it);
    }

    return -1;
}

int StylesModel::selectedStyleIndex() const
{
    return indexOfStyle(m_selectedStyle);
}

QString StylesModel::styleConfigPage(const QString &style) const
{
    const int idx = indexOfStyle(style);
    if (idx == -1) {
        return QString();
    }

    return m_data.at(idx).configPage;
}

void StylesModel::load()
{
    beginResetModel();

    const int oldCount = m_data.count();

    m_data.clear();

    // Combines the info we get from QStyleFactory and our themerc files
    QHash<QString, StylesModelData> styleData;

    const QStringList allStyles = QStyleFactory::keys();
    for (const QString &styleName : allStyles) {
        auto &item = styleData[styleName];
        item.styleName = styleName;
    }

    QStringList themeFiles;

    const QStringList themeDirs =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("kstyle/themes"), QStandardPaths::LocateDirectory);
    for (const QString &dir : themeDirs) {
        const QStringList fileNames = QDir(dir).entryList(QStringList{QStringLiteral("*.themerc")});
        for (const QString &file : fileNames) {
            const QString suffixedFileName = QLatin1String("kstyle/themes/") + file;
            if (!themeFiles.contains(suffixedFileName)) {
                themeFiles.append(suffixedFileName);
            }
        }
    }

    std::transform(themeFiles.begin(), themeFiles.end(), themeFiles.begin(), [](const QString &item) {
        return QStandardPaths::locate(QStandardPaths::GenericDataLocation, item);
    });

    for (const QString &file : themeFiles) {
        KConfig config(file, KConfig::SimpleConfig);
        if (!config.hasGroup("KDE") || !config.hasGroup("Misc")) {
            continue;
        }

        KConfigGroup kdeGroup = config.group("KDE");

        const QString styleName = kdeGroup.readEntry("WidgetStyle", QString());
        if (styleName.isEmpty()) {
            continue;
        }

        auto it = styleData.find(styleName);
        if (it == styleData.end()) {
            continue;
        }

        auto &item = *it;

        KConfigGroup desktopEntryGroup = config.group("Desktop Entry");
        if (desktopEntryGroup.readEntry("Hidden", false)) {
            // Don't list hidden styles
            styleData.remove(styleName);
            continue;
        }

        KConfigGroup miscGroup = config.group("Misc");

        item.display = miscGroup.readEntry("Name");
        item.description = miscGroup.readEntry("Comment");
        item.configPage = miscGroup.readEntry("ConfigPage");
    }

    m_data = styleData.values().toVector();

    // Sort case-insensitively
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_data.begin(), m_data.end(), [&collator](const StylesModelData &a, const StylesModelData &b) {
        const QString aDisplay = !a.display.isEmpty() ? a.display : a.styleName;
        const QString bDisplay = !b.display.isEmpty() ? b.display : b.styleName;
        return collator.compare(aDisplay, bDisplay) < 0;
    });

    endResetModel();

    // an item might have been added before the currently selected one
    if (oldCount != m_data.count()) {
        Q_EMIT selectedStyleIndexChanged();
    }
}
