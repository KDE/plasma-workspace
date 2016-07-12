/********************************************************************
Copyright 2016  Eike Hein <hein.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#ifndef TASKTOOLS_H
#define TASKTOOLS_H

#include "taskmanager_export.h"

#include <QIcon>
#include <QModelIndex>
#include <QUrl>

namespace TaskManager
{

struct AppData
{
    QString id; // Application id (*.desktop sans extension).
    QString name; // Application name.
    QString genericName; // Generic application name.
    QIcon icon;
    QUrl url;
};

enum UrlComparisonMode {
     Strict = 0,
     IgnoreQueryItems
};

/**
 * Fills in and returns an AppData struct based on the given URL.
 *
 * If the URL contains iconData in its query string, it is decoded and
 * set as AppData.icon, taking precedence over normal icon discovery.
 *
 * If the URL is using the preferred:// scheme, the URL it resolves to
 * is set as AppData.url.
 *
 * The supplied fallback icon is set as AppData.icon if no other icon
 * could be found.
 *
 * @see defaultApplication
 * @param url A URL to a .desktop file or executable, or a preferred:// URL.
 * @param fallbackIcon An icon to use when none could be read from the URL or
 * otherwise found.
 * @returns @c AppData filled in based on the given URL.
 */
TASKMANAGER_EXPORT AppData appDataFromUrl(const QUrl &url, const QIcon &fallbackIcon = QIcon());

/**
 * Returns an application id for an URL using the preferred:// scheme.
 *
 * Recognized values for the host component of the URL are:
 * - "browser"
 * - "mailer"
 * - "terminal"
 * - "windowmanager"
 *
 * If the host component matches none of the above, an attempt is made
 * to match to application links stored in kcm_componentchooser/.
 *
 * @param url A URL using the preferred:// scheme.
 * @returns an application id for the given URL.
 **/
TASKMANAGER_EXPORT QString defaultApplication(const QUrl &url);

/**
 * Convenience function to compare two launcher URLs either strictly
 * or ignoring their query strings.
 *
 * @see LauncherTasksModel
 * @param a The first launcher URL.
 * @param b The second launcher URL.
 * @param c The comparison mode. Either Strict or IgnoreQueryItems.
 * @returns @c true if the URLs match.
 **/
TASKMANAGER_EXPORT bool launcherUrlsMatch(const QUrl &a, const QUrl &b, UrlComparisonMode mode = Strict);

/**
 * Determines whether tasks model entries belong to the same app.
 *
 * @param a The first model index.
 * @param b The second model index.
 * @returns @c true if the model entries belong to the same app.
 **/
TASKMANAGER_EXPORT bool appsMatch(const QModelIndex &a, const QModelIndex &b);

/**
 * Given global coordinates, returns the geometry of the screen they are
 * on, or the geometry of the screen they are closest to.
 *
 * @param pos Coordinates in global space.
 * @return The geometry of the screen containing pos or closest to pos.
 */
TASKMANAGER_EXPORT QRect screenGeometry(const QPoint &pos);
}

#endif
