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

#include <KDirWatch>
#include <KGlobal>
#include <KGlobalSettings>
#include <KPluginFactory>
#include <KPluginLoader>

#include <KUrl>

#include <kdirnotify.h>
#include <QStandardPaths>


K_PLUGIN_FACTORY_WITH_JSON(DesktopNotifierFactory,
                           "desktopnotifier.json",
                           registerPlugin<DesktopNotifier>();)

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
    if (path == QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/user-dirs.dirs")){
        checkDesktopLocation();
    }
}

void DesktopNotifier::dirty(const QString &path)
{
    Q_UNUSED(path)

    if (path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + '/' + "Trash/files")) {
        // Trigger an update of the trash icon
        if (QFile::exists(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/trash.desktop"))
            org::kde::KDirNotify::emitFilesChanged(QList<QUrl>() << QUrl(QStringLiteral("desktop:/trash.desktop")));
    } else if (path == QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/user-dirs.dirs")){
        checkDesktopLocation();
    } else {
        // Emitting FilesAdded forces a re-read of the dir
        KUrl url("desktop:/");
        url.addPath(KUrl::relativePath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), path));
        url.cleanPath();
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
