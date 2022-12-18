/*
    SPDX-FileCopyrightText: 2008, 2009 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kio_desktop.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <KDirNotify>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QStorageInfo>

#include "desktopnotifier_interface.h"
#include "kded_interface.h"

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.desktop" FILE "desktop.json")
};

extern "C" {
int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    // necessary to use other kio workers
    QCoreApplication app(argc, argv);
    app.setApplicationName("kio_desktop");

    // start the worker
    DesktopProtocol worker(argv[1], argv[2], argv[3]);
    worker.dispatchLoop();
    return 0;
}
}

DesktopProtocol::DesktopProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app)
    : KIO::ForwardingWorkerBase(protocol, pool, app)
{
    checkLocalInstall();

    org::kde::kded5 kded(QStringLiteral("org.kde.kded5"), QStringLiteral("/kded"), QDBusConnection::sessionBus());
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
        QFile::copy(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kio_desktop/directory.desktop")), desktopPath + "/.directory");

        // Copy the desktop links
        QSet<QString> links;
        const auto dirs =
            QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("kio_desktop/DesktopLinks"), QStandardPaths::LocateDirectory);
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
    QString oldPath = url.path();
    // So that creating a new folder while at "desktop:" (without a '/' after ':')
    // doesn't create "/home/user/DesktopNew Folder" instead of "/home/user/Desktop/New Folder".
    if (oldPath.isEmpty() || !oldPath.startsWith(QLatin1Char('/'))) {
        oldPath.prepend(QLatin1Char('/'));
    }

    newUrl.setScheme(QStringLiteral("file"));
    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    newUrl.setPath(desktopPath + oldPath);
    newUrl = newUrl.adjusted(QUrl::StripTrailingSlash);

    return true;
}

KIO::WorkerResult DesktopProtocol::listDir(const QUrl &url)
{
    KIO::WorkerResult res = KIO::ForwardingWorkerBase::listDir(url);

    QUrl actual;
    rewriteUrl(url, actual);

    org::kde::DesktopNotifier kded(QStringLiteral("org.kde.kded5"), QStringLiteral("/modules/desktopnotifier"), QDBusConnection::sessionBus());
    kded.watchDir(actual.path());

    return res;
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

void DesktopProtocol::adjustUDSEntry(KIO::UDSEntry &entry, UDSEntryCreationMode creationMode) const
{
    KIO::ForwardingWorkerBase::adjustUDSEntry(entry, creationMode);
    const QString path = desktopFile(entry);

    if (!path.isEmpty()) {
        KDesktopFile file(path);

        const QString name = file.readName();
        if (!name.isEmpty())
            entry.replace(KIO::UDSEntry::UDS_DISPLAY_NAME, name);

        if (!file.tryExec())
            entry.replace(KIO::UDSEntry::UDS_HIDDEN, 1);
    }

    // Set a descriptive display name for the root item
    if (requestedUrl().path() == QLatin1String("/") && entry.stringValue(KIO::UDSEntry::UDS_NAME) == QLatin1Char('.')) {
        entry.replace(KIO::UDSEntry::UDS_DISPLAY_NAME, i18n("Desktop Folder"));
    }

    // Set the target URL to the local path
    QUrl localUrl(QUrl::fromLocalFile(entry.stringValue(KIO::UDSEntry::UDS_LOCAL_PATH)));
    entry.replace(KIO::UDSEntry::UDS_TARGET_URL, localUrl.toString());
}

KIO::WorkerResult DesktopProtocol::rename(const QUrl &_src, const QUrl &_dest, KIO::JobFlags flags)
{
    Q_UNUSED(flags)

    if (_src == _dest) {
        return KIO::WorkerResult::pass();
    }

    QUrl src;
    rewriteUrl(_src, src);
    const QString srcPath = src.toLocalFile();

    QUrl dest;
    rewriteUrl(_dest, dest);
    QString destPath = dest.toLocalFile();
    QUrl reported_dest = _dest;

    if (KDesktopFile::isDesktopFile(srcPath)) {
        QString friendlyName;

        if (destPath.endsWith(QLatin1String(".desktop"))) {
            const QString fileName = dest.fileName();
            friendlyName = KIO::decodeFileName(fileName.left(fileName.length() - 8));
        } else {
            friendlyName = KIO::decodeFileName(dest.fileName());
            destPath.append(QLatin1String(".desktop"));
            reported_dest.setPath(reported_dest.path().append(QLatin1String(".desktop")));
        }

        // Update the value of the Name field in the file.
        KDesktopFile file(src.toLocalFile());
        KConfigGroup cg(file.desktopGroup());
        cg.writeEntry("Name", friendlyName);
        cg.writeEntry("Name", friendlyName, KConfigGroup::Persistent | KConfigGroup::Localized);
        cg.sync();
    }

    if (QFile(srcPath).rename(destPath)) {
        org::kde::KDirNotify::emitFileRenamedWithLocalPath(_src, reported_dest, destPath);
        return KIO::WorkerResult::pass();
    } else {
        return KIO::WorkerResult::fail(KIO::ERR_CANNOT_RENAME, srcPath);
    }
}

KIO::WorkerResult DesktopProtocol::fileSystemFreeSpace(const QUrl &url)
{
    Q_UNUSED(url)

    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QStorageInfo storageInfo{desktopPath};
    if (storageInfo.isValid() && storageInfo.isReady()) {
        setMetaData(QStringLiteral("total"), QString::number(storageInfo.bytesTotal()));
        setMetaData(QStringLiteral("available"), QString::number(storageInfo.bytesAvailable()));
        return KIO::WorkerResult::pass();
    } else {
        return KIO::WorkerResult::fail(KIO::ERR_CANNOT_STAT, desktopPath);
    }
}

#include "kio_desktop.moc"
