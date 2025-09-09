/*
    SPDX-FileCopyrightText: 2005-2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QDir>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <private/qtx11extras_p.h>

#include "cursorthemesettings.h"
#include "thememodel.h"
#include "xcursortheme.h"

#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>

using namespace Qt::StringLiterals;

// Check for older version
#if !defined(XCURSOR_LIB_MAJOR) && defined(XCURSOR_MAJOR)
#define XCURSOR_LIB_MAJOR XCURSOR_MAJOR
#define XCURSOR_LIB_MINOR XCURSOR_MINOR
#endif

Q_LOGGING_CATEGORY(KCM_CURSORTHEME, "kcm_cursortheme", QtWarningMsg)

CursorThemeModel::CursorThemeModel(QObject *parent)
    : QAbstractListModel(parent)
{
    insertThemes();
}

CursorThemeModel::~CursorThemeModel()
{
    qDeleteAll(list);
    list.clear();
}

QHash<int, QByteArray> CursorThemeModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
    roleNames[CursorTheme::DisplayDetailRole] = QByteArrayLiteral("description");
    roleNames[CursorTheme::IsWritableRole] = QByteArrayLiteral("isWritable");
    roleNames[CursorTheme::PendingDeletionRole] = QByteArrayLiteral("pendingDeletion");

    return roleNames;
}

void CursorThemeModel::refreshList()
{
    beginResetModel();
    qDeleteAll(list);
    list.clear();
    defaultName.clear();
    endResetModel();
    insertThemes();
}

QVariant CursorThemeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= list.count())
        return {};

    CursorTheme *theme = list.at(index.row());

    // Text label
    if (role == Qt::DisplayRole) {
        return theme->title();
    }

    // Description for the first name column
    if (role == CursorTheme::DisplayDetailRole)
        return theme->description();

    // Icon for the name column
    if (role == Qt::DecorationRole)
        return theme->icon();

    if (role == CursorTheme::IsWritableRole) {
        return theme->isWritable();
    }

    if (role == CursorTheme::PendingDeletionRole) {
        return pendingDeletions.contains(theme);
    }

    return {};
}

bool CursorThemeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return false;
    }
    if (role == CursorTheme::PendingDeletionRole) {
        const bool shouldRemove = value.toBool();
        if (shouldRemove) {
            pendingDeletions.push_back(list[index.row()]);
        } else {
            pendingDeletions.removeAll(list[index.row()]);
        }
        Q_EMIT dataChanged(index, index, {role});
        return true;
    }
    return false;
}

void CursorThemeModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column);
    Q_UNUSED(order);

    // Sorting of the model isn't implemented, as the KCM currently uses
    // a sorting proxy model.
}

const CursorTheme *CursorThemeModel::theme(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;

    if (index.row() < 0 || index.row() >= list.count())
        return nullptr;

    return list.at(index.row());
}

QModelIndex CursorThemeModel::findIndex(const QString &name)
{
    uint hash = qHash(name);

    for (int i = 0; i < list.count(); i++) {
        const CursorTheme *theme = list.at(i);
        if (theme->hash() == hash)
            return index(i, 0);
    }

    return {};
}

QModelIndex CursorThemeModel::defaultIndex()
{
    return findIndex(defaultName);
}

const QStringList CursorThemeModel::searchPaths()
{
    if (!baseDirs.isEmpty())
        return baseDirs;

#if XCURSOR_LIB_MAJOR == 1 && XCURSOR_LIB_MINOR < 1
    // These are the default paths Xcursor will scan for cursor themes
    QString path("~/.icons:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons");

    // If XCURSOR_PATH is set, use that instead of the default path
    char *xcursorPath = std::getenv("XCURSOR_PATH");
    if (xcursorPath)
        path = xcursorPath;
#else
    // Get the search path from Xcursor
    QString path = QString::fromLocal8Bit(XcursorLibraryPath());
    qCDebug(KCM_CURSORTHEME) << "XcursorLibraryPath:" << path;
#endif

    // Separate the paths
    baseDirs = path.split(u':', Qt::SkipEmptyParts);

    // Remove duplicates
    QMutableStringListIterator i(baseDirs);
    while (i.hasNext()) {
        const QString path = i.next();
        QMutableStringListIterator j(i);
        while (j.hasNext())
            if (j.next() == path)
                j.remove();
    }

    // Expand all occurrences of ~/ to the home dir
    baseDirs.replaceInStrings(QRegularExpression(u"^~\\/"_s), QDir::home().path() + u'/');
    return baseDirs;
}

bool CursorThemeModel::hasTheme(const QString &name) const
{
    const uint hash = qHash(name);

    for (const CursorTheme *theme : std::as_const(list))
        if (theme->hash() == hash)
            return true;

    return false;
}

bool CursorThemeModel::handleDefault(const QDir &themeDir)
{
    QFileInfo info(themeDir.path());

    // If "default" is a symlink
    if (info.isSymLink()) {
        QFileInfo target(info.symLinkTarget());
        if (target.exists() && (target.isDir() || target.isSymLink()))
            defaultName = target.fileName();

        return true;
    }

    // If there's no cursors subdir, or if it's empty
    if (!themeDir.exists(QStringLiteral("cursors")) || QDir(themeDir.path() + "/cursors"_L1).entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty()) {
        if (themeDir.exists(QStringLiteral("index.theme"))) {
            XCursorTheme theme(themeDir);
            if (!theme.inherits().isEmpty())
                defaultName = theme.inherits().at(0);
        }
        return true;
    }

    defaultName = QStringLiteral("default");
    return false;
}

void CursorThemeModel::processThemeDir(const QDir &themeDir, const QStringList &whitelist)
{
    qCDebug(KCM_CURSORTHEME) << "Searching in" << themeDir;
    bool haveCursors = themeDir.exists(QStringLiteral("cursors"));

    // Special case handling of "default", since it's usually either a
    // symlink to another theme, or an empty theme that inherits another
    // theme.
    if (defaultName.isNull() && themeDir.dirName() == QLatin1String("default")) {
        if (handleDefault(themeDir))
            return;
    }

    // If the directory lacks an index.theme file it can't be a cursor theme.
    // If the directory has an index.theme but not a cursors subdir, then although
    // it might be a valid cursor theme (by inheriting another cursor theme), for
    // the purpose of this KCM it's a duplicate of the inherited theme, so we'll
    // skip it (unless it's in the whitelist)
    if ((!themeDir.exists(QStringLiteral("index.theme")) || !haveCursors) && !whitelist.contains(themeDir.dirName()))
        return;

    // Create a cursor theme object for the theme dir
    auto *theme = new XCursorTheme(themeDir);

    // Skip this theme if it's hidden.
    if (theme->isHidden()) {
        delete theme;
        return;
    }

    // Append the theme to the list
    beginInsertRows(QModelIndex(), list.size(), list.size());
    list.append(theme);
    endInsertRows();
}

void CursorThemeModel::insertThemes()
{
    // Always show the active theme and the default theme, even if they are just duplicates of other themes
    const CursorThemeSettings settings;
    const QStringList whitelist{settings.cursorTheme(), settings.defaultCursorThemeValue()};

    // Scan each base dir for Xcursor themes and add them to the list.
    const QStringList paths{searchPaths()};
    qCDebug(KCM_CURSORTHEME) << "searchPaths:" << paths;
    for (const QString &baseDir : paths) {
        QDir dir(baseDir);
        if (!dir.exists())
            continue;

        // Process each subdir in the directory
        for (const auto list{dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)}; const QString &name : list) {
            // Don't process the theme if a theme with the same name already exists
            // in the list. Xcursor will pick the first one it finds in that case,
            // and since we use the same search order, the one Xcursor picks should
            // be the one already in the list.
            if (hasTheme(name) || !dir.cd(name))
                continue;

            processThemeDir(dir, whitelist);
            dir.cdUp(); // Return to the base dir
        }
    }

    // The theme Xcursor will end up using if no theme is configured
    if (defaultName.isNull() || !hasTheme(defaultName))
        defaultName = QStringLiteral("KDE_Classic");
}

bool CursorThemeModel::addTheme(const QDir &dir)
{
    auto *theme = new XCursorTheme(dir);

    // Don't add the theme to the list if it's hidden
    if (theme->isHidden()) {
        delete theme;
        return false;
    }

    // ### If the theme is hidden, the user will probably find it strange that it
    //     doesn't appear in the list view. There also won't be a way for the user
    //     to delete the theme using the KCM. Perhaps a warning about this should
    //     be issued, and the user be given a chance to undo the installation.

    // If an item with the same name already exists in the list,
    // we'll remove it before inserting the new one.
    for (int i = 0; i < list.count(); i++) {
        if (list.at(i)->hash() == theme->hash()) {
            removeTheme(index(i, 0));
            break;
        }
    }

    // Append the theme to the list
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    list.append(theme);
    endInsertRows();

    return true;
}

void CursorThemeModel::removeTheme(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    beginRemoveRows(QModelIndex(), index.row(), index.row());
    pendingDeletions.removeAll(list[index.row()]);
    delete list.takeAt(index.row());
    endRemoveRows();
}
