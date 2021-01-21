/* This file is part of the KDE project
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>

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

#include <KLocalizedString>
#include <kio/slavebase.h>
#include <kservice.h>
#include <kservicegroup.h>
#include <sys/stat.h>
#include <time.h>

#include <QStandardPaths>
#include <QUrl>

class ApplicationsProtocol : public KIO::SlaveBase
{
public:
    enum RunMode {
        ProgramsMode,
        ApplicationsMode,
    };
    ApplicationsProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app);
    ~ApplicationsProtocol() override;
    void get(const QUrl &url) override;
    void stat(const QUrl &url) override;
    void listDir(const QUrl &url) override;

private:
    RunMode m_runMode;
};

extern "C" {
Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("kio_applications");

    ApplicationsProtocol slave(argv[1], argv[2], argv[3]);
    slave.dispatchLoop();
    return 0;
}
}

static void createFileEntry(KIO::UDSEntry &entry, const KService::Ptr &service, const QUrl &parentUrl)
{
    entry.clear();
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, KIO::encodeFileName(service->name()));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    const QString fileUrl = parentUrl.url() + '/' + service->desktopEntryName();
    entry.fastInsert(KIO::UDSEntry::UDS_URL, fileUrl);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, 0500);
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("application/x-desktop"));
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    const QString localPath = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, QStringLiteral("%1.desktop").arg(service->desktopEntryName()));
    entry.fastInsert(KIO::UDSEntry::UDS_LOCAL_PATH, localPath);
    entry.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME, time(nullptr));
    entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, service->icon());
}

static void createDirEntry(KIO::UDSEntry &entry, const QString &name, const QString &url, const QString &mime, const QString &iconName)
{
    entry.clear();
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, name);
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, 0500);
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, mime);
    if (!url.isEmpty())
        entry.fastInsert(KIO::UDSEntry::UDS_URL, url);
    entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, iconName);
}

ApplicationsProtocol::ApplicationsProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app)
    : SlaveBase(protocol, pool, app)
{
    // Adjusts which part of the K Menu to virtualize.
    if (protocol == "programs")
        m_runMode = ProgramsMode;
    else // if (protocol == "applications")
        m_runMode = ApplicationsMode;
}

ApplicationsProtocol::~ApplicationsProtocol()
{
}

void ApplicationsProtocol::get(const QUrl &url)
{
    KService::Ptr service = KService::serviceByDesktopName(url.fileName());
    if (service && service->isValid()) {
        const QString localPath = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, QStringLiteral("%1.desktop").arg(service->desktopEntryName()));
        QUrl redirUrl(QUrl::fromLocalFile(localPath));
        redirection(redirUrl);
        finished();
    } else {
        error(KIO::ERR_IS_DIRECTORY, url.toDisplayString());
    }
}

void ApplicationsProtocol::stat(const QUrl &url)
{
    KIO::UDSEntry entry;

    QString servicePath(url.path());
    if (!servicePath.endsWith('/'))
        servicePath.append('/');
    servicePath.remove(0, 1); // remove starting '/'

    KServiceGroup::Ptr grp = KServiceGroup::group(servicePath);

    if (grp && grp->isValid()) {
        createDirEntry(entry,
                       ((m_runMode == ApplicationsMode) ? i18n("Applications") : i18n("Programs")),
                       url.url(),
                       QStringLiteral("inode/directory"),
                       grp->icon());
    } else {
        KService::Ptr service = KService::serviceByDesktopName(url.fileName());
        if (service && service->isValid()) {
            createFileEntry(entry, service, url);
        } else {
            error(KIO::ERR_SLAVE_DEFINED, i18n("Unknown application folder"));
            return;
        }
    }

    statEntry(entry);
    finished();
}

void ApplicationsProtocol::listDir(const QUrl &url)
{
    QString groupPath = url.path();
    if (!groupPath.endsWith('/'))
        groupPath.append('/');
    groupPath.remove(0, 1); // remove starting '/'

    KServiceGroup::Ptr grp = KServiceGroup::group(groupPath);

    if (!grp || !grp->isValid()) {
        error(KIO::ERR_DOES_NOT_EXIST, groupPath);
        return;
    }

    unsigned int count = 0;
    KIO::UDSEntry entry;

    foreach (const KSycocaEntry::Ptr &e, grp->entries(true, true)) {
        if (e->isType(KST_KServiceGroup)) {
            KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e.data()));

            // qDebug() << "ADDING SERVICE GROUP WITH PATH " << g->relPath();

            // Avoid adding empty groups.
            KServiceGroup::Ptr subMenuRoot = KServiceGroup::group(g->relPath());
            if (subMenuRoot->childCount() == 0)
                continue;

            // Ignore dotfiles.
            if (g->name().startsWith('.'))
                continue;

            QString relPath = g->relPath();
            QUrl dirUrl = url; // preserve protocol, whether that's programs:/ or applications:/
            dirUrl.setPath('/' + relPath);
            dirUrl = dirUrl.adjusted(QUrl::StripTrailingSlash);
            // qDebug() << "ApplicationsProtocol: adding entry" << dirUrl;
            createDirEntry(entry, g->caption(), dirUrl.url(), QStringLiteral("inode/directory"), g->icon());
        } else {
            KService::Ptr service(static_cast<KService *>(e.data()));

            // qDebug() << "the entry name is" << service->desktopEntryName()
            //         << "with path" << service->entryPath();

            if (!service->isApplication()) // how could this happen?
                continue;
            createFileEntry(entry, service, url);
        }

        listEntry(entry);
        count++;
    }

    totalSize(count);
    finished();
}
