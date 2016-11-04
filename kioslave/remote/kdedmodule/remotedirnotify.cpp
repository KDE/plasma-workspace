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

#include "../debug.h"
#include <kdesktopfile.h>
#include <kdirnotify.h>
#include <KIO/Global>

#include <QtDBus/QtDBus>

RemoteDirNotify::RemoteDirNotify()
{
	const QString path = QStringLiteral("%1/remoteview").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
	m_baseURL.setPath(path);

	QDBusConnection::sessionBus().connect(QString(), QString(), QStringLiteral("org.kde.KDirNotify"),
				    QStringLiteral("FilesAdded"), this, SLOT(FilesAdded(QString)));
	QDBusConnection::sessionBus().connect(QString(), QString(), QStringLiteral("org.kde.KDirNotify"),
				    QStringLiteral("FilesRemoved"), this, SLOT(FilesRemoved(QStringList)));
	QDBusConnection::sessionBus().connect(QString(), QString(), QStringLiteral("org.kde.KDirNotify"),
				    QStringLiteral("FilesChanged"), this, SLOT(FilesChanged(QStringList)));
}

QUrl RemoteDirNotify::toRemoteURL(const QUrl &url)
{
	qCDebug(KIOREMOTE_LOG) << "RemoteDirNotify::toRemoteURL(" << url << ")";
	if ( m_baseURL.isParentOf(url) )
	{
		QString path = QDir(m_baseURL.path()).relativeFilePath(url.path());
		QUrl result;
		result.setScheme(QStringLiteral("remote"));
		result.setPath(path);
		result.setPath(QDir::cleanPath(result.path()));
		qCDebug(KIOREMOTE_LOG) << "result => " << result;
		return result;
	}

	qCDebug(KIOREMOTE_LOG) << "result => QUrl()";
	return QUrl();
}

QList<QUrl> RemoteDirNotify::toRemoteURLList(const QStringList &list)
{
    QList<QUrl> urls;
    for (const QString &file : list) {
        QUrl url = toRemoteURL(QUrl::fromLocalFile(file));
        if (url.isValid()) {
            urls.append(url);
        }
    }

    return urls;
}

void RemoteDirNotify::FilesAdded(const QString &directory)
{
	qCDebug(KIOREMOTE_LOG) << "RemoteDirNotify::FilesAdded";

	QUrl new_dir = toRemoteURL(QUrl::fromLocalFile(directory));

	if (new_dir.isValid())
	{
		org::kde::KDirNotify::emitFilesAdded(new_dir);
	}
}

// This hack is required because of the way we manage .desktop files with
// Forwarding Slaves, their URL is out of the ioslave (most remote:/ files
// have a file:/ based UDS_URL so that they are executed correctly.
// Hence, FilesRemoved and FilesChanged does nothing... We're forced to use
// FilesAdded to re-list the modified directory.
inline void evil_hack(const QList<QUrl> &list)
{
	QList<QUrl> notified;

	QList<QUrl>::const_iterator it = list.begin();
	QList<QUrl>::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		QUrl url = KIO::upUrl(*it);

		if (!notified.contains(url))
		{
			org::kde::KDirNotify::emitFilesAdded(url);
			notified.append(url);
		}
	}
}


void RemoteDirNotify::FilesRemoved(const QStringList &fileList)
{
	qCDebug(KIOREMOTE_LOG) << "RemoteDirNotify::FilesRemoved";

	QList<QUrl> new_list = toRemoteURLList(fileList);

	if (!new_list.isEmpty())
	{
		//KDirNotify_stub notifier("*", "*");
		//notifier.FilesRemoved( new_list );
		evil_hack(new_list);
	}
}

void RemoteDirNotify::FilesChanged(const QStringList &fileList)
{
	qCDebug(KIOREMOTE_LOG) << "RemoteDirNotify::FilesChanged";

	QList<QUrl> new_list = toRemoteURLList(fileList);

	if (!new_list.isEmpty())
	{
		//KDirNotify_stub notifier("*", "*");
		//notifier.FilesChanged( new_list );
		evil_hack(new_list);
	}
}

