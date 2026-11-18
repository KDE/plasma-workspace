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

#include "kded_interface.h"

using namespace Qt::StringLiterals;

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
    app.setApplicationName(u"kio_desktop"_s);

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

    org::kde::kded6 kded(QStringLiteral("org.kde.kded6"), QStringLiteral("/kded"), QDBusConnection::sessionBus());
    auto pending = kded.loadModule(u"desktopnotifier"_s);
    pending.waitForFinished();
}

DesktopProtocol::~DesktopProtocol() = default;

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
        ::mkdir(QFile::encodeName(desktopPath).constData(), S_IRWXU);
        desktopIsEmpty = true;
    } else
        desktopIsEmpty = desktopDir.entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty();

    if (desktopIsEmpty) {
        // Copy the .directory file
        QFile::copy(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kio_desktop/directory.desktop")),
                    desktopPath + "/.directory"_L1);

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

        for (const QString &link : std::as_const(links)) {
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

    Q_EMIT redirection(newUrl);

    return true;
}


#include "kio_desktop.moc"

#include "moc_kio_desktop.cpp"
