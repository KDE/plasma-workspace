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

static QList<int> gtkVersionsList(bool gtk2, bool gtk3, bool gtk4)
{
    QList<int> versions;
    if (gtk2) {
        versions << 2;
    }
    if (gtk3) {
        versions << 3;
    }
    if (gtk4) {
        versions << 4;
    }
    return versions;
}

GtkThemesModel::GtkThemesModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_selectedTheme(QStringLiteral("Breeze"))
    , m_themes()
{
}

void GtkThemesModel::load()
{
    QMap<QString, GtkTheme> themes;

    for (const QString &possibleThemePath : possiblePathsToThemes()) {
        QDir possibleThemeDirectory(possibleThemePath);
        const auto hasSubdir = [&possibleThemeDirectory](const QString &pattern) {
            return !possibleThemeDirectory.entryList({pattern}, QDir::Dirs).isEmpty();
        };

        GtkTheme theme;
        theme.path = possibleThemeDirectory.path();
        theme.gtk2 = hasSubdir(QStringLiteral("gtk-2.0"));
        theme.gtk3 = hasSubdir(QStringLiteral("gtk-3.*"));
        theme.gtk4 = hasSubdir(QStringLiteral("gtk-4.*"));

        if (theme.gtk2 || theme.gtk3 || theme.gtk4) {
            // Do not show dark Breeze GTK variant, since the colors of it
            // are coming from the color scheme and selecting them here
            // is redundant and does not work
            if (possibleThemeDirectory.dirName() == u"Breeze-Dark") {
                continue;
            }
            // Setting gtk-theme-name to "Default" breaks dark theme, Adwaita is listed instead
            if (possibleThemeDirectory.dirName() == u"Default") {
                continue;
            }
            themes.insert(possibleThemeDirectory.dirName(), theme);
        }
    }

    // Built into GTK, may have no theme directory
    GtkTheme &adwaita = themes[QStringLiteral("Adwaita")];
    adwaita.gtk3 = true;
    adwaita.gtk4 = true;

    setThemesList(themes);
}

QString GtkThemesModel::themePath(const QString &themeName)
{
    return m_themes.value(themeName).path;
}

QVariant GtkThemesModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index)) {
        return {};
    }

    auto it = m_themes.constBegin();
    std::advance(it, index.row());

    switch (role) {
    case Qt::DisplayRole:
    case Roles::ThemeNameRole:
        return it.key();
    case Roles::ThemePathRole:
        return it.value().path;
    case Roles::GtkVersionsRole:
        return QVariant::fromValue(gtkVersionsList(it.value().gtk2, it.value().gtk3, it.value().gtk4));
    default:
        return {};
    }
}

QHash<int, QByteArray> GtkThemesModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles[Roles::ThemeNameRole] = QByteArrayLiteral("theme-name");
    roles[Roles::ThemePathRole] = QByteArrayLiteral("theme-path");
    roles[Roles::GtkVersionsRole] = QByteArrayLiteral("gtk-versions");

    return roles;
}

int GtkThemesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_themes.count();
}

void GtkThemesModel::setThemesList(const QMap<QString, GtkTheme> &themes)
{
    beginResetModel();
    m_themes = themes;
    endResetModel();
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

bool GtkThemesModel::selectedThemeSupportsGtk4() const
{
    return m_themes.value(m_selectedTheme).gtk4;
}

QList<int> GtkThemesModel::selectedThemeMissingGtkVersions() const
{
    const auto it = m_themes.constFind(m_selectedTheme);
    if (it == m_themes.constEnd()) {
        return {};
    }
    return gtkVersionsList(!it->gtk2, !it->gtk3, !it->gtk4);
}

QStringList GtkThemesModel::possiblePathsToThemes()
{
    QStringList possibleThemesPaths;

    QStringList themesLocationsPaths =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("themes"), QStandardPaths::LocateDirectory);
    // TODO: Remove. See https://gitlab.gnome.org/GNOME/gtk/-/issues/6219 for when GTK plans to, and when we should,
    // remove it as well.
    themesLocationsPaths << QDir::homePath() + QStringLiteral("/.themes");

    for (const QString &themesLocationPath : std::as_const(themesLocationsPaths)) {
        const QStringList possibleThemesDirectoriesNames = QDir(themesLocationPath).entryList(QDir::NoDotAndDotDot | QDir::AllDirs);
        for (const QString &possibleThemeDirectoryName : possibleThemesDirectoriesNames) {
            possibleThemesPaths += themesLocationPath + u'/' + possibleThemeDirectoryName;
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

#include "moc_gtkthemesmodel.cpp"
