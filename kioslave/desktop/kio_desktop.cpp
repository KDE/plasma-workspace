/* This file is part of the KDE project
   Copyright (C) 2008, 2009 Fredrik Höglund <fredrik@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kio_desktop.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <KDirNotify>
#include <KDiskFreeSpaceInfo>
#include <KIO/UDSEntry>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include "kded_interface.h"
#include "desktopnotifier_interface.h"

extern "C"
{
    int Q_DECL_EXPORT kdemain(int argc, char **argv)
    {
        // necessary to use other kio slaves
        QCoreApplication app(argc, argv);
        app.setApplicationName("kio_desktop");

        // start the slave
        DesktopProtocol slave(argv[1], argv[2], argv[3]);
        slave.dispatchLoop();
        return 0;
    }
}

DesktopProtocol::DesktopProtocol(const QByteArray& protocol, const QByteArray &pool, const QByteArray &app)
    : KIO::ForwardingSlaveBase(protocol, pool, app)
{
    checkLocalInstall();

    org::kde::kded5 kded(QStringLiteral("org.kde.kded5"),
                        QStringLiteral("/kded"),
                        QDBusConnection::sessionBus());
    auto pending = kded.loadModule("desktopnotifier");
    pending.waitForFinished();
}

DesktopProtocol::~DesktopProtocol()
{
}

void DesktopProtocol::checkLocalInstall()
{
#ifndef Q_OS_WIN
    // QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) returns the home dir
    // if the desktop folder doesn't exist, so verify its result
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    const QDir desktopDir(desktopPath);
    bool desktopIsEmpty;

    // Create the desktop folder if it doesn't exist
    if (!desktopDir.exists()) {
        ::mkdir(QFile::encodeName(desktopPath), S_IRWXU);
        desktopIsEmpty = true;
    } else
        desktopIsEmpty = desktopDir.entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty();

    if (desktopIsEmpty) {
        // Copy the .directory file
        QFile::copy(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kio_desktop/directory.desktop")),
                    desktopPath + "/.directory");

        // Copy the desktop links
        QSet<QString> links;
        const auto dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("kio_desktop/DesktopLinks"), QStandardPaths::LocateDirectory);
        for (const auto &dir : dirs) {
            const auto fileNames = QDir(dir).entryList({QStringLiteral("*.desktop")});
            for (const auto &file : fileNames) {
                links += file;
            }
        }

        foreach (const QString &link, links) {
            const auto fullPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kio_desktop/DesktopLinks/%1").arg(link));
            KDesktopFile file(fullPath);
            if (!file.desktopGroup().readEntry("Hidden", false))
                QFile::copy(fullPath, QStringLiteral("%1/%2").arg(desktopPath, link));
        }
    }
#endif
}

bool DesktopProtocol::rewriteUrl(const QUrl &url, QUrl &newUrl)
{
    newUrl.setScheme(QStringLiteral("file"));
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.endsWith('/')) {
        desktopPath.chop(1);
    }
    QString filePath = desktopPath + url.path();
    if (filePath.endsWith('/')) {
        filePath.chop(1); // ForwardingSlaveBase always appends a '/'
    }
    newUrl.setPath(filePath);
    return true;
}

void DesktopProtocol::listDir(const QUrl &url)
{
    KIO::ForwardingSlaveBase::listDir(url);

    QUrl actual;
    rewriteUrl(url, actual);

    org::kde::DesktopNotifier kded(QStringLiteral("org.kde.kded5"), QStringLiteral("/modules/desktopnotifier"), QDBusConnection::sessionBus());
    kded.watchDir(actual.path());
}

QString DesktopProtocol::desktopFile(KIO::UDSEntry &entry) const
{
    const QString name = entry.stringValue(KIO::UDSEntry::UDS_NAME);
    if (name == QLatin1Char('.') || name == QLatin1String(".."))
        return QString();

    QUrl url = processedUrl();
    url.setPath(QStringLiteral("%1/%2").arg(url.path(), name));

    if (entry.isDir()) {
        url.setPath(QStringLiteral("%1/.directory").arg(url.path()));
        if (!QFileInfo::exists(url.path()))
            return QString();

        return url.path();
    }

    if (KDesktopFile::isDesktopFile(url.path()))
        return url.path();

    return QString();
}

void DesktopProtocol::prepareUDSEntry(KIO::UDSEntry &entry, bool listing) const
{
    ForwardingSlaveBase::prepareUDSEntry(entry, listing);
    const QString path = desktopFile(entry);

    if (!path.isEmpty()) {
        KDesktopFile file(path);

        const QString name = file.readName();
        if (!name.isEmpty())
            entry.replace(KIO::UDSEntry::UDS_DISPLAY_NAME, name);

        if (file.noDisplay() || !file.tryExec())
            entry.replace(KIO::UDSEntry::UDS_HIDDEN, 1);
    }

    // Set a descriptive display name for the root item
    if (requestedUrl().path() == QLatin1String("/")
        && entry.stringValue(KIO::UDSEntry::UDS_NAME) == QLatin1Char('.')) {
        entry.replace(KIO::UDSEntry::UDS_DISPLAY_NAME, i18n("Desktop Folder"));
    }

    // Set the target URL to the local path 
    QUrl localUrl(QUrl::fromLocalFile(entry.stringValue(KIO::UDSEntry::UDS_LOCAL_PATH)));
    entry.replace(KIO::UDSEntry::UDS_TARGET_URL, localUrl.toString());
}

void DesktopProtocol::rename(const QUrl &_src, const QUrl &_dest, KIO::JobFlags flags)
{
    Q_UNUSED(flags)

    if (_src == _dest) {
        finished();
        return;
    }

    QUrl src;
    rewriteUrl(_src, src);
    const QString srcPath = src.toLocalFile();

    QUrl dest;
    rewriteUrl(_dest, dest);
    const QString destPath = dest.toLocalFile();

    if (KDesktopFile::isDesktopFile(srcPath)) {
        QString friendlyName;

        if (destPath.endsWith(QLatin1String(".desktop"))) {
            const QString fileName = dest.fileName();
            friendlyName = KIO::decodeFileName(fileName.left(fileName.length() - 8));
        } else {
            friendlyName = KIO::decodeFileName(dest.fileName());
        }

        // Update the value of the Name field in the file.
        KDesktopFile file(src.toLocalFile());
        KConfigGroup cg(file.desktopGroup());
        cg.writeEntry("Name", friendlyName);
        cg.writeEntry("Name", friendlyName, KConfigGroup::Persistent | KConfigGroup::Localized);
        cg.sync();
    }

    if (QFile(srcPath).rename(destPath)) {
        org::kde::KDirNotify::emitFileRenamedWithLocalPath(_src, _dest, destPath);
        finished();
    } else {
        error(KIO::ERR_CANNOT_RENAME, srcPath);
    }
}

void DesktopProtocol::virtual_hook(int id, void *data)
{
    switch(id) {
        case SlaveBase::GetFileSystemFreeSpace: {
            QUrl *url = static_cast<QUrl *>(data);
            fileSystemFreeSpace(*url);
        }   break;
        default:
            SlaveBase::virtual_hook(id, data);
    }
}

void DesktopProtocol::fileSystemFreeSpace(const QUrl &url)
{
    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    const KDiskFreeSpaceInfo spaceInfo = KDiskFreeSpaceInfo::freeSpaceInfo(desktopPath);
    if (spaceInfo.isValid()) {
        setMetaData(QStringLiteral("total"), QString::number(spaceInfo.size()));
        setMetaData(QStringLiteral("available"), QString::number(spaceInfo.available()));
        finished();
    } else {
        error(KIO::ERR_CANNOT_STAT, desktopPath);
    }
}
