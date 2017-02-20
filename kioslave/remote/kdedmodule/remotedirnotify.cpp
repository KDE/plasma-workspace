/* This file is part of the KDE Project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>

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

#include "remotedirnotify.h"

#include <KDirNotify>
#include <KDirWatch>

#include <QStandardPaths>
#include <QUrl>

RemoteDirNotify::RemoteDirNotify()
{
    const QString path = QStringLiteral("%1/remoteview").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));

    m_dirWatch = new KDirWatch(this);
    m_dirWatch->addDir(path, KDirWatch::WatchFiles);

    connect(m_dirWatch, &KDirWatch::created, this, &RemoteDirNotify::slotRemoteChanged);
    connect(m_dirWatch, &KDirWatch::deleted, this, &RemoteDirNotify::slotRemoteChanged);
    connect(m_dirWatch, &KDirWatch::dirty, this, &RemoteDirNotify::slotRemoteChanged);
}

void RemoteDirNotify::slotRemoteChanged()
{
    org::kde::KDirNotify::emitFilesAdded(QUrl("remote:/"));
}
