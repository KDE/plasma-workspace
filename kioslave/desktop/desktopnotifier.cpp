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
#include <KStandardDirs>
#include <KUrl>

#include <kdirnotify.h>
#include <QStandardPaths>


K_PLUGIN_FACTORY(DesktopNotifierFactory, registerPlugin<DesktopNotifier>();)
K_EXPORT_PLUGIN(DesktopNotifierFactory("kio_desktop"))


DesktopNotifier::DesktopNotifier(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    dirWatch = new KDirWatch(this);
    dirWatch->addDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    dirWatch->addDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + '/' + "Trash/files");

    connect(dirWatch, &KDirWatch::dirty, this, &DesktopNotifier::dirty);
}

void DesktopNotifier::watchDir(const QString &path)
{
    dirWatch->addDir(path);
}

void DesktopNotifier::dirty(const QString &path)
{
    Q_UNUSED(path)

    if (path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + '/' + "Trash/files")) {
        // Trigger an update of the trash icon
        if (QFile::exists(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/trash.desktop"))
            org::kde::KDirNotify::emitFilesChanged(QList<QUrl>() << QUrl("desktop:/trash.desktop"));
    } else {
        // Emitting FilesAdded forces a re-read of the dir
        KUrl url("desktop:/");
        url.addPath(KUrl::relativePath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), path));
        url.cleanPath();
        org::kde::KDirNotify::emitFilesAdded(url);
    }
}

#include <desktopnotifier.moc>
