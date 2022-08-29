/*
    SPDX-FileCopyrightText: 2020 Mikhail Zolotukhin <zomial@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

#include <KIO/DeleteJob>

#include "gtkthemesmodel.h"

GtkThemesModel::GtkThemesModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_selectedTheme(QStringLiteral("Breeze"))
    , m_themes()
{
}

void GtkThemesModel::load()
{
    QMap<QString, QString> gtk3ThemesNames;

    static const QStringList gtk3SubdirPattern(QStringLiteral("gtk-3.*"));
    for (const QString &possibleThemePath : possiblePathsToThemes()) {
        // If the directory contains any of gtk-3.X folders, it is the GTK3 theme for sure
        QDir possibleThemeDirectory(possibleThemePath);
        if (!possibleThemeDirectory.entryList(gtk3SubdirPattern, QDir::Dirs).isEmpty()) {
            // Do not show dark Breeze GTK variant, since the colors of it
            // are coming from the color scheme and selecting them here
            // is redundant and does not work
            if (possibleThemeDirectory.dirName() == QStringLiteral("Breeze-Dark")) {
                continue;
            }

            gtk3ThemesNames.insert(possibleThemeDirectory.dirName(), possibleThemeDirectory.path());
        }
    }

    setThemesList(gtk3ThemesNames);
}

QString GtkThemesModel::themePath(const QString &themeName)
{
    if (themeName.isEmpty()) {
        return QString();
    } else {
        return m_themes.constFind(themeName).value();
    }
}

QVariant GtkThemesModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index)) {
        return QVariant();
    }

    const auto &item = m_themes.constBegin() + index.row();

    switch (role) {
    case Qt::DisplayRole:
    case Roles::ThemeNameRole:
        return item.key();
    case Roles::ThemePathRole:
        return item.value();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> GtkThemesModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles[Roles::ThemeNameRole] = "theme-name";
    roles[Roles::ThemePathRole] = "theme-path";

    return roles;
}

int GtkThemesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_themes.count();
}

void GtkThemesModel::setThemesList(const QMap<QString, QString> &themes)
{
    beginResetModel();
    m_themes = themes;
    endResetModel();
}

QMap<QString, QString> GtkThemesModel::themesList()
{
    return m_themes;
}

void GtkThemesModel::setSelectedTheme(const QString &themeName)
{
    m_selectedTheme = themeName;
    Q_EMIT selectedThemeChanged(themeName);
}

QString GtkThemesModel::selectedTheme()
{
    return m_selectedTheme;
}

QStringList GtkThemesModel::possiblePathsToThemes()
{
    QStringList possibleThemesPaths;

    QStringList themesLocationsPaths =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("themes"), QStandardPaths::LocateDirectory);
    themesLocationsPaths << QDir::homePath() + QStringLiteral("/.themes");

    for (const QString &themesLocationPath : qAsConst(themesLocationsPaths)) {
        const QStringList possibleThemesDirectoriesNames = QDir(themesLocationPath).entryList(QDir::NoDotAndDotDot | QDir::AllDirs);
        for (const QString &possibleThemeDirectoryName : possibleThemesDirectoriesNames) {
            possibleThemesPaths += themesLocationPath + '/' + possibleThemeDirectoryName;
        }
    }

    return possibleThemesPaths;
}

bool GtkThemesModel::selectedThemeRemovable()
{
    return themePath(m_selectedTheme).contains(QDir::homePath());
}

void GtkThemesModel::removeSelectedTheme()
{
    QString path = themePath(m_selectedTheme);
    KIO::DeleteJob *deleteJob = KIO::del(QUrl::fromLocalFile(path), KIO::HideProgressInfo);
    connect(deleteJob, &KJob::finished, this, [this]() {
        Q_EMIT themeRemoved();
    });
}

int GtkThemesModel::findThemeIndex(const QString &themeName)
{
    return static_cast<int>(std::distance(m_themes.constBegin(), m_themes.constFind(themeName)));
}

void GtkThemesModel::setSelectedThemeDirty()
{
    Q_EMIT selectedThemeChanged(m_selectedTheme);
}
