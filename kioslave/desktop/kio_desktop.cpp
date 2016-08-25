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

#include <KApplication>
#include <KCmdLineArgs>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KDirNotify>
#include <KGlobalSettings>
#include <KStandardDirs>
#include <KGlobal>
#include <KUrl>
#include <kdeversion.h>

#include <kio/udsentry.h>
#include <kio_version.h>

#include <QFile>
#include <QDBusInterface>
#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>

extern "C"
{
    int Q_DECL_EXPORT kdemain(int argc, char **argv)
    {
        // necessary to use other kio slaves
        QCoreApplication app(argc, argv);
        KComponentData("kio_desktop", "kdelibs4");
        KLocale::global();

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

    QDBusInterface kded(QStringLiteral("org.kde.kded5"), QStringLiteral("/kded"), QStringLiteral("org.kde.kded5"));
    kded.call(QStringLiteral("loadModule"), "desktopnotifier");
}

DesktopProtocol::~DesktopProtocol()
{
}

void DesktopProtocol::checkLocalInstall()
{
#ifndef Q_WS_WIN
    // We can't use QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) here, since it returns the home dir
    // if the desktop folder doesn't exist.
    QString desktopPath = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
    if (desktopPath.isEmpty())
        desktopPath = QDir::homePath() + "/Desktop";

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

        // Copy the trash link
        QFile::copy(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kio_desktop/directory.trash")),
                    desktopPath + "/trash.desktop");
 
        // Copy the desktop links
        const QStringList links = KGlobal::dirs()->findAllResources("data", QStringLiteral("kio_desktop/DesktopLinks/*"),
                                                                    KStandardDirs::NoDuplicates);
        foreach (const QString &link, links) {
            KDesktopFile file(link);
            if (!file.desktopGroup().readEntry("Hidden", false))
                QFile::copy(link, desktopPath + link.mid(link.lastIndexOf('/')));
        }
    }
#endif
}

bool DesktopProtocol::rewriteUrl(const QUrl &url, QUrl &newUrl)
{
    newUrl.setScheme(QStringLiteral("file"));
    newUrl.setPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + url.path());
    return true;
}

void DesktopProtocol::listDir(const QUrl &url)
{
    KIO::ForwardingSlaveBase::listDir(url);

    KUrl actual;
    rewriteUrl(url, actual);

    QDBusInterface kded(QStringLiteral("org.kde.kded5"), QStringLiteral("/modules/desktopnotifier"), QStringLiteral("org.kde.DesktopNotifier"));
    kded.call(QStringLiteral("watchDir"), actual.path());
}

QString DesktopProtocol::desktopFile(KIO::UDSEntry &entry) const
{
    const QString name = entry.stringValue(KIO::UDSEntry::UDS_NAME);
    if (name == QLatin1String(".") || name == QLatin1String(".."))
        return QString();

    KUrl url = processedUrl();
    url.addPath(name);

    if (entry.isDir()) {
        url.addPath(QStringLiteral(".directory"));
        if (!KStandardDirs::exists(url.path()))
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
            entry.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, name);

        if (file.noDisplay() || !file.tryExec())
            entry.insert(KIO::UDSEntry::UDS_HIDDEN, 1);
    }

    // Set the target URL to the local path 
    QUrl localUrl(QUrl::fromLocalFile(entry.stringValue(KIO::UDSEntry::UDS_LOCAL_PATH)));
    entry.insert(KIO::UDSEntry::UDS_TARGET_URL, localUrl.toString());
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
#if KIO_VERSION >= QT_VERSION_CHECK(5, 20, 0)
        org::kde::KDirNotify::emitFileRenamedWithLocalPath(_src, _dest, destPath);
#else
        org::kde::KDirNotify::emitFileRenamed(_src, _dest);
#endif
        finished();
    } else {
        error(KIO::ERR_CANNOT_RENAME, srcPath);
    }
}

