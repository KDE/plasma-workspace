/********************************************************************
Copyright 2016  Eike Hein <hein.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "tasktools.h"
#include "abstracttasksmodel.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <kemailsettings.h>
#include <KFileItem>
#include <KMimeTypeTrader>
#include <KRun>
#include <KSharedConfig>

#include <QDir>
#include <QGuiApplication>
#include <QScreen>

namespace TaskManager
{

AppData appDataFromUrl(const QUrl &url, const QIcon &fallbackIcon)
{
    AppData data;
    data.url = url;

    if (url.hasQuery()) {
        QUrlQuery uQuery(url);

        if (uQuery.hasQueryItem(QLatin1String("iconData"))) {
            QString iconData(uQuery.queryItemValue(QLatin1String("iconData")));
            QPixmap pixmap;
            QByteArray bytes = QByteArray::fromBase64(iconData.toLocal8Bit(), QByteArray::Base64UrlEncoding);
            pixmap.loadFromData(bytes);
            data.icon.addPixmap(pixmap);
        }
    }

    if (url.isLocalFile() && KDesktopFile::isDesktopFile(url.toLocalFile())) {
        KDesktopFile f(url.toLocalFile());

        const KService::Ptr service = KService::serviceByStorageId(f.fileName());

        if (service && QUrl::fromLocalFile(service->entryPath()) == url) {
            data.name = service->name();
            data.genericName = service->genericName();
            data.id = service->storageId();

            if (data.icon.isNull()) {
                data.icon = QIcon::fromTheme(service->icon());
            }
        } else if (f.tryExec()) {
            data.name = f.readName();
            data.genericName = f.readGenericName();
            data.id = QUrl::fromLocalFile(f.fileName()).fileName();

            if (data.icon.isNull()) {
                data.icon = QIcon::fromTheme(f.readIcon());
            }
        }

        if (data.id.endsWith(".desktop")) {
            data.id = data.id.left(data.id.length() - 8);
        }
    } else if (url.scheme() == QLatin1String("preferred")) {
        data.id = defaultApplication(url);

        const KService::Ptr service = KService::serviceByStorageId(data.id);

        if (service) {
            QString desktopFile = service->entryPath();

            // Update with resolved URL.
            data.url = QUrl::fromLocalFile(desktopFile);

            KDesktopFile f(desktopFile);
            KConfigGroup cg(&f, "Desktop Entry");

            data.icon = QIcon::fromTheme(f.readIcon());
            const QString exec = cg.readEntry("Exec", QString());
            data.name = cg.readEntry("Name", QString());

            if (data.name.isEmpty() && !exec.isEmpty()) {
                data.name = exec.split(' ').at(0);
            }

            data.genericName = f.readGenericName();
        }
    }

    if (data.name.isEmpty()) {
        data.name = url.fileName();
    }

    if (data.icon.isNull()) {
        data.icon = fallbackIcon;
    }

    return data;
}

QString defaultApplication(const QUrl &url)
{
    if (url.scheme() != QLatin1String("preferred")) {
        return QString();
    }

    const QString &application = url.host();

    if (application.isEmpty()) {
        return QString();
    }

    if (application.compare(QLatin1String("mailer"), Qt::CaseInsensitive) == 0) {
        KEMailSettings settings;

        // In KToolInvocation, the default is kmail; but let's be friendlier.
        QString command = settings.getSetting(KEMailSettings::ClientProgram);

        if (command.isEmpty()) {
            if (KService::Ptr kontact = KService::serviceByStorageId(QStringLiteral("kontact"))) {
                return kontact->storageId();
            } else if (KService::Ptr kmail = KService::serviceByStorageId(QStringLiteral("kmail"))) {
                return kmail->storageId();
            }
        }

        if (!command.isEmpty()) {
            if (settings.getSetting(KEMailSettings::ClientTerminal) == QLatin1String("true")) {
                KConfigGroup confGroup(KSharedConfig::openConfig(), "General");
                const QString preferredTerminal = confGroup.readPathEntry("TerminalApplication",
                                                  QStringLiteral("konsole"));
                command = preferredTerminal + QLatin1String(" -e ") + command;
            }

            return command;
        }
    } else if (application.compare(QLatin1String("browser"), Qt::CaseInsensitive) == 0) {
        KConfigGroup config(KSharedConfig::openConfig(), "General");
        QString browserApp = config.readPathEntry("BrowserApplication", QString());

        if (browserApp.isEmpty()) {
            const KService::Ptr htmlApp = KMimeTypeTrader::self()->preferredService(QStringLiteral("text/html"));

            if (htmlApp) {
                browserApp = htmlApp->storageId();
            }
        } else if (browserApp.startsWith('!')) {
            browserApp = browserApp.mid(1);
        }

        return browserApp;
    } else if (application.compare(QLatin1String("terminal"), Qt::CaseInsensitive) == 0) {
        KConfigGroup confGroup(KSharedConfig::openConfig(), "General");

        return confGroup.readPathEntry("TerminalApplication", QStringLiteral("konsole"));
    } else if (application.compare(QLatin1String("filemanager"), Qt::CaseInsensitive) == 0) {
        KService::Ptr service = KMimeTypeTrader::self()->preferredService(QStringLiteral("inode/directory"));

        if (service) {
            return service->storageId();
        }
    } else if (KService::Ptr service = KMimeTypeTrader::self()->preferredService(application)) {
        return service->storageId();
    } else {
        // Try the files in share/apps/kcm_componentchooser/*.desktop.
        QStringList directories = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("kcm_componentchooser"), QStandardPaths::LocateDirectory);
        QStringList services;

        foreach(const QString& directory, directories) {
            QDir dir(directory);
            foreach(const QString& f, dir.entryList(QStringList("*.desktop")))
                services += dir.absoluteFilePath(f);
        }

        foreach (const QString & service, services) {
            KConfig config(service, KConfig::SimpleConfig);
            KConfigGroup cg = config.group(QByteArray());
            const QString type = cg.readEntry("valueName", QString());

            if (type.compare(application, Qt::CaseInsensitive) == 0) {
                KConfig store(cg.readPathEntry("storeInFile", QStringLiteral("null")));
                KConfigGroup storeCg(&store, cg.readEntry("valueSection", QString()));
                const QString exec = storeCg.readPathEntry(cg.readEntry("valueName", "kcm_componenchooser_null"),
                                     cg.readEntry("defaultImplementation", QString()));

                if (!exec.isEmpty()) {
                    return exec;
                }

                break;
            }
        }
    }

    return QString("");
}

bool launcherUrlsMatch(const QUrl &a, const QUrl &b, UrlComparisonMode mode)
{
    if (mode == IgnoreQueryItems) {
        return (a.adjusted(QUrl::RemoveQuery) == b.adjusted(QUrl::RemoveQuery));
    }

    return (a == b);
}

bool appsMatch(const QModelIndex &a, const QModelIndex &b)
{
    const QString &aAppId = a.data(AbstractTasksModel::AppId).toString();
    const QString &bAppId = b.data(AbstractTasksModel::AppId).toString();

    if (!aAppId.isEmpty() && aAppId == bAppId) {
        return true;
    }

    const QUrl &aUrl = a.data(AbstractTasksModel::LauncherUrlWithoutIcon).toUrl();
    const QUrl &bUrl = b.data(AbstractTasksModel::LauncherUrlWithoutIcon).toUrl();

    if (aUrl.isValid() && aUrl == bUrl) {
        return true;
    }

    return false;
}

QRect screenGeometry(const QPoint &pos)
{
    if (pos.isNull()) {
        return QRect();
    }

    const QList<QScreen *> &screens = QGuiApplication::screens();
    QRect screenGeometry;
    int shortestDistance = INT_MAX;

    for (int i = 0; i < screens.count(); ++i) {
        const QRect &geometry = screens.at(i)->geometry();

        if (geometry.contains(pos)) {
            return geometry;
        }

        int distance = QPoint(geometry.topLeft() - pos).manhattanLength();
        distance = qMin(distance, QPoint(geometry.topRight() - pos).manhattanLength());
        distance = qMin(distance, QPoint(geometry.bottomRight() - pos).manhattanLength());
        distance = qMin(distance, QPoint(geometry.bottomLeft() - pos).manhattanLength());

        if (distance < shortestDistance) {
            shortestDistance = distance;
            screenGeometry = geometry;
        }
    }

    return screenGeometry;
}

}
