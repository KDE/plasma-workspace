/* This file is part of the KDE Project
   Copyright (C) 2008, 2009 Fredrik Höglund <fredrik@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "desktopnotifier.h"

#include <KDesktopFile>
#include <KDirWatch>
#include <KPluginFactory>

#include <kdirnotify.h>

#include <QDir>
#include <QFile>
#include <QStandardPaths>

K_PLUGIN_CLASS_WITH_JSON(DesktopNotifier, "desktopnotifier.json")

DesktopNotifier::DesktopNotifier(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    m_desktopLocation = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

    dirWatch = new KDirWatch(this);
    dirWatch->addDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    dirWatch->addDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + '/' + "Trash/files");
    dirWatch->addFile(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/user-dirs.dirs"));

    connect(dirWatch, &KDirWatch::created, this, &DesktopNotifier::created);
    connect(dirWatch, &KDirWatch::dirty, this, &DesktopNotifier::dirty);
}

void DesktopNotifier::watchDir(const QString &path)
{
    dirWatch->addDir(path);
}

void DesktopNotifier::created(const QString &path)
{
    if (path == QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/user-dirs.dirs")) {
        checkDesktopLocation();
    }
}

void DesktopNotifier::dirty(const QString &path)
{
    Q_UNUSED(path)

    if (path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + '/' + "Trash/files")) {
        QList<QUrl> trashUrls;

        // Check for any .desktop file linking to trash:/ to update its icon
        const auto desktopFiles = QDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).entryInfoList({QStringLiteral("*.desktop")});
        for (const auto &fi : desktopFiles) {
            KDesktopFile df(fi.absoluteFilePath());
            if (df.hasLinkType() && df.readUrl() == QLatin1String("trash:/")) {
                trashUrls << QUrl(QStringLiteral("desktop:/") + fi.fileName());
            }
        }

        if (!trashUrls.isEmpty()) {
            org::kde::KDirNotify::emitFilesChanged(trashUrls);
        }
    } else if (path == QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/user-dirs.dirs")) {
        checkDesktopLocation();
    } else {
        // Emitting FilesAdded forces a re-read of the dir
        QUrl url;
        url.setScheme(QStringLiteral("desktop"));
        const auto relativePath = QDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).relativeFilePath(path);
        url.setPath(QStringLiteral("%1/%2").arg(url.path(), relativePath));
        url.setPath(QDir::cleanPath(url.path()));
        org::kde::KDirNotify::emitFilesAdded(url);
    }
}

void DesktopNotifier::checkDesktopLocation()
{
    const QUrl &currentLocation = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

    if (m_desktopLocation != currentLocation) {
        m_desktopLocation = currentLocation;
        org::kde::KDirNotify::emitFilesChanged(QList<QUrl>() << QUrl(QStringLiteral("desktop:/")));
    }
}

#include <desktopnotifier.moc>
