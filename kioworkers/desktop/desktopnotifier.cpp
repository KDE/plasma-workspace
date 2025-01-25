/*
    SPDX-FileCopyrightText: 2008, 2009 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "desktopnotifier.h"

#include <KDesktopFile>
#include <KPluginFactory>

#include <kdirnotify.h>

#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QStandardPaths>

K_PLUGIN_CLASS_WITH_JSON(DesktopNotifier, "desktopnotifier.json")

using namespace Qt::StringLiterals;

DesktopNotifier::DesktopNotifier(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    m_desktopLocation = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

    watcher = new QFileSystemWatcher(this);
    watcher->addPaths(QStringList{QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                  QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + u"/trashrc",
                                  QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/user-dirs.dirs")});

    connect(watcher, &QFileSystemWatcher::fileChanged, this, &DesktopNotifier::dirty);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &DesktopNotifier::dirty);
}

void DesktopNotifier::watchDir(const QString &path)
{
    watcher->addPath(path);
}

void DesktopNotifier::dirty(const QString &path)
{
    if (path == QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + u"/trashrc") {
        QList<QUrl> trashUrls;

        // Check for any .desktop file linking to trash:/ to update its icon
        const auto desktopFiles = QDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).entryInfoList({QStringLiteral("*.desktop")});
        for (const auto &fi : desktopFiles) {
            KDesktopFile df(fi.absoluteFilePath());
            if (df.hasLinkType() && df.readUrl() == QLatin1String("trash:/")) {
                trashUrls << QUrl(QString(u"desktop:/" + fi.fileName()));
            }
        }

        if (!trashUrls.isEmpty()) {
            org::kde::KDirNotify::emitFilesChanged(trashUrls);
        }

        if (!watcher->files().contains(path)) {
            watcher->addPath(path);
        }
    } else if (path == QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/user-dirs.dirs")) {
        checkDesktopLocation();

        if (!watcher->files().contains(path)) {
            watcher->addPath(path);
        }
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
        org::kde::KDirNotify::emitFilesChanged(QList{QUrl(QStringLiteral("desktop:/"))});
    }
}

#include <desktopnotifier.moc>

#include "moc_desktopnotifier.cpp"
