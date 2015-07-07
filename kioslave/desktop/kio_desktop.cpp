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
#include <KGlobalSettings>
#include <KStandardDirs>
#include <KGlobal>
#include <KUrl>
#include <kdeversion.h>

#include <kio/udsentry.h>

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

    QDBusInterface kded("org.kde.kded5", "/kded", "org.kde.kded5");
    kded.call("loadModule", "desktopnotifier");
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
    bool newRelease;

    // Check if we have a new KDE release
    KConfig config("kio_desktoprc");
    KConfigGroup cg(&config, "General");
    QString version = cg.readEntry("Version", "0.0.0");
    int major = version.section('.', 0, 0).toInt();
    int minor = version.section('.', 1, 1).toInt();
    int release = version.section('.', 2, 2).toInt();

    if (KDE_MAKE_VERSION(major, minor, release) < KDE::version()) {
        const QString version = QString::number(KDE::versionMajor()) + '.' +
                                QString::number(KDE::versionMinor()) + '.' +
                                QString::number(KDE::versionRelease());
        cg.writeEntry("Version", version);
        newRelease = true;
    } else 
        newRelease = false;

    // Create the desktop folder if it doesn't exist
    if (!desktopDir.exists()) {
        ::mkdir(QFile::encodeName(desktopPath), S_IRWXU);
        desktopIsEmpty = true;
    } else
        desktopIsEmpty = desktopDir.entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty();

    if (desktopIsEmpty) {
        // Copy the .directory file
        QFile::copy(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kio_desktop/directory.desktop"),
                    desktopPath + "/.directory");

        // Copy the trash link
        QFile::copy(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kio_desktop/directory.trash"),
                    desktopPath + "/trash.desktop");
 
        // Copy the desktop links
        const QStringList links = KGlobal::dirs()->findAllResources("data", "kio_desktop/DesktopLinks/*",
                                                                    KStandardDirs::NoDuplicates);
        foreach (const QString &link, links) {
            KDesktopFile file(link);
            if (!file.desktopGroup().readEntry("Hidden", false))
                QFile::copy(link, desktopPath + link.mid(link.lastIndexOf('/')));
        }
    } else if (newRelease) {
        // Update the icon name in the .directory file to the FDO naming spec
        const QString directoryFile = desktopPath + "/.directory";
        if (QFile::exists(directoryFile)) {
             KDesktopFile file(directoryFile);
             if (file.readIcon() == "desktop")
                 file.desktopGroup().writeEntry("Icon", "user-desktop");
        } else
             QFile::copy(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kio_desktop/directory.desktop"), directoryFile);
  
        // Update the home icon to the FDO naming spec
        const QString homeLink = desktopPath + "/Home.desktop";
        if (QFile::exists(homeLink)) {
            KDesktopFile home(homeLink);
            const QString icon = home.readIcon();
            if (icon == "kfm_home" || icon == "folder_home")
                home.desktopGroup().writeEntry("Icon", "user-home");
        }

        // Update the trash icon to the FDO naming spec  
        const QString trashLink = desktopPath + "/trash.desktop";
        if (QFile::exists(trashLink)) {
            KDesktopFile trash(trashLink);
            if (trash.readIcon() == "trashcan_full")
                trash.desktopGroup().writeEntry("Icon", "user-trash-full");
            if (trash.desktopGroup().readEntry("EmptyIcon") == "trashcan_empty")
                trash.desktopGroup().writeEntry("EmptyIcon", "user-trash");
        }
    }
#endif
}

bool DesktopProtocol::rewriteUrl(const QUrl &url, QUrl &newUrl)
{
    newUrl.setScheme("file");
    newUrl.setPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + '/' + url.path());
    return true;
}

void DesktopProtocol::listDir(const QUrl &url)
{
    KIO::ForwardingSlaveBase::listDir(url);

    KUrl actual;
    rewriteUrl(url, actual);

    QDBusInterface kded("org.kde.kded5", "/modules/desktopnotifier", "org.kde.DesktopNotifier");
    kded.call("watchDir", actual.path());
}

QString DesktopProtocol::desktopFile(KIO::UDSEntry &entry) const
{
    const QString name = entry.stringValue(KIO::UDSEntry::UDS_NAME);
    if (name == "." || name == "..")
        return QString();

    KUrl url = processedUrl();
    url.addPath(name);

    if (entry.isDir()) {
        url.addPath(".directory");
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
    entry.insert(KIO::UDSEntry::UDS_TARGET_URL, entry.stringValue(KIO::UDSEntry::UDS_LOCAL_PATH));
}

void DesktopProtocol::rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags)
{
    KUrl url;
    rewriteUrl(src, url);

    if (src.scheme() != "desktop" || dest.scheme() != "desktop" ||
        !KDesktopFile::isDesktopFile(url.path()))
    {
        ForwardingSlaveBase::rename(src, dest, flags);
        return;
    }

    QString friendlyName;
    KUrl destUrl = dest;  

    if (dest.url().endsWith(".desktop")) {
        const QString fileName = dest.fileName(); 
        friendlyName = KIO::decodeFileName(fileName.left(fileName.length() - 8));
    } else {
        friendlyName = KIO::decodeFileName(dest.fileName());
        destUrl.setFileName(destUrl.fileName() + ".desktop");
    }

    // Update the value of the Name field in the file.
    KDesktopFile file(url.path());
    KConfigGroup cg(file.desktopGroup());
    cg.writeEntry("Name", friendlyName);
    cg.writeEntry("Name", friendlyName, KConfigGroup::Persistent | KConfigGroup::Localized);
    cg.sync();

    if (src.url() != destUrl.url())
        ForwardingSlaveBase::rename(src, destUrl, flags);
    else
        finished();
}

