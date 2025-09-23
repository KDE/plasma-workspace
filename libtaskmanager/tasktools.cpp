/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "tasktools.h"
#include "abstracttasksmodel.h"

#include <ranges>

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KFileItem>
#include <KNotificationJobUiDelegate>
#include <KProcessList>
#include <KWindowSystem>
#include <PlasmaActivities/ResourceInstance>
#include <kemailsettings.h>

#include <KIO/ApplicationLauncherJob>
#include <KIO/OpenUrlJob>

#include <QDir>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QScreen>
#include <QUrlQuery>
#include <qnamespace.h>

#include <defaultservice.h>

using namespace Qt::StringLiterals;

static const QString appropriateCaption(const KService::Ptr &service)
{
    if (!service) {
        return {};
    }

    const QString genericName = service->genericName();
    if (!genericName.isEmpty() && genericName != service->name()) {
        return genericName;
    }

    return service->comment();
}

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

    // applications: URLs are used to refer to applications by their KService::menuId
    // (i.e. .desktop file name) rather than the absolute path to a .desktop file.
    if (url.scheme() == QLatin1String("applications")) {
        const KService::Ptr service = KService::serviceByMenuId(url.path());

        if (service && url.path() == service->menuId()) {
            data.name = service->name();
            data.genericName = appropriateCaption(service);
            data.id = service->storageId();

            if (data.icon.isNull()) {
                data.icon = QIcon::fromTheme(service->icon());
            }
        }
    }

    if (url.isLocalFile()) {
        if (KDesktopFile::isDesktopFile(url.toLocalFile())) {
            const KService::Ptr service = KService::serviceByStorageId(url.fileName());

            // Resolve to non-absolute menuId-based URL if possible.
            if (service) {
                const QString &menuId = service->menuId();

                if (!menuId.isEmpty()) {
                    data.url = QUrl(QLatin1String("applications:") + menuId);
                }
            }

            if (service && QUrl::fromLocalFile(service->entryPath()) == url) {
                data.name = service->name();
                data.genericName = appropriateCaption(service);
                data.id = service->storageId();

                if (data.icon.isNull()) {
                    data.icon = QIcon::fromTheme(service->icon());
                }
            } else {
                KDesktopFile f(url.toLocalFile());
                if (f.tryExec()) {
                    data.name = f.readName();
                    data.genericName = appropriateCaption(KService::serviceByDesktopPath(url.toLocalFile()));
                    data.id = QUrl::fromLocalFile(f.fileName()).fileName();

                    if (data.icon.isNull()) {
                        const QString iconValue = f.readIcon();
                        if (QIcon::hasThemeIcon(iconValue)) {
                            data.icon = QIcon::fromTheme(iconValue);
                        } else if (!iconValue.startsWith(QDir::separator())) {
                            const int lastIndexOfPeriod = iconValue.lastIndexOf(QLatin1Char('.'));
                            const QString iconValueWithoutSuffix = lastIndexOfPeriod < 0 ? iconValue : iconValue.left(lastIndexOfPeriod);
                            // Find an icon in the same folder
                            const QDir sameDir = QFileInfo(url.toLocalFile()).absoluteDir();
                            const auto iconList = sameDir.entryInfoList(
                                {
                                    QStringLiteral("*.png").arg(iconValueWithoutSuffix),
                                    QStringLiteral("*.svg").arg(iconValueWithoutSuffix),
                                },
                                QDir::Files);
                            if (!iconList.empty()) {
                                data.icon = QIcon(iconList[0].absoluteFilePath());
                            }
                        } else {
                            data.icon = QIcon(iconValue);
                        }
                    }
                }
            }

            if (data.id.endsWith(u".desktop")) {
                data.id = data.id.left(data.id.length() - 8);
            }
        } else {
            data.id = url.fileName();
        }

    } else if (url.scheme() == QLatin1String("preferred")) {
        data.id = defaultApplication(url);

        const KService::Ptr service = KService::serviceByStorageId(data.id);

        if (service) {
            const QString &menuId = service->menuId();
            const QString &desktopFile = service->entryPath();

            data.name = service->name();
            data.genericName = appropriateCaption(service);
            data.id = service->storageId();

            if (data.icon.isNull()) {
                data.icon = QIcon::fromTheme(service->icon());
            }

            // Update with resolved URL.
            if (!menuId.isEmpty()) {
                data.url = QUrl(QLatin1String("applications:") + menuId);
            } else {
                data.url = QUrl::fromLocalFile(desktopFile);
            }
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

QUrl windowUrlFromMetadata(const QString &appId, quint32 pid, const QString &xWindowsWMClassName)
{
    QUrl url;
    KService::List services;
    bool triedPid = false;

    // The code below this function goes on a hunt for services based on the metadata
    // that has been passed in. Occasionally, it will find more than one matching
    // service. In some scenarios (e.g. multiple identically-named .desktop files)
    // there's a need to pick the most useful one. The function below promises to "sort"
    // a list of services by how closely their KService::menuId() relates to the key that
    // has been passed in. The current naive implementation simply looks for a menuId
    // that starts with the key, prepends it to the list and returns it. In practice,
    // that means a KService with a menuId matching the appId will win over one with a
    // menuId that encodes a subfolder hierarchy.
    // A concrete example: Valve's Steam client is sometimes installed two times, once
    // natively as a Linux application, once via Wine. Both have .desktop files named
    // (S|)steam.desktop. The Linux native version is located in the menu by means of
    // categorization ("Games") and just has a menuId() matching the .desktop file name,
    // but the Wine version is placed in a folder hierarchy by Wine and gets a menuId()
    // of wine-Programs-Steam-Steam.desktop. The weighing done by this function makes
    // sure the Linux native version gets mapped to the former, while other heuristics
    // map the Wine version reliably to the latter.
    // In lieu of this weighing we just used whatever KApplicationTrader returned first,
    // so what we do here can be no worse.
    auto sortServicesByMenuId = [](KService::List &services, const QString &key) {
        if (services.count() == 1) {
            return;
        }

        for (const auto &service : services) {
            if (service->menuId().startsWith(key, Qt::CaseInsensitive)) {
                services.prepend(service);
                return;
            }
        }
    };

    if (!(appId.isEmpty() && xWindowsWMClassName.isEmpty())) {
        if (!appId.isEmpty()) {
            // Try matching both appId and xWindowsWMClassName against StartupWMClass.
            // We do this before evaluating the mapping rules further, because StartupWMClass
            // is essentially a mapping rule, and we expect it to be set deliberately and
            // sensibly to instruct us what to do. Also, mapping rules
            //
            // StartupWMClass=STRING
            //
            //   If true, it is KNOWN that the application will map at least one
            //   window with the given string as its WM class or WM name hint.
            //
            // Source: https://specifications.freedesktop.org/startup-notification-spec/startup-notification-0.1.txt
            if (services.isEmpty() && !xWindowsWMClassName.isEmpty()) {
                services = KApplicationTrader::query([&xWindowsWMClassName](const KService::Ptr &service) {
                    return service->property<QString>(QStringLiteral("StartupWMClass")).compare(xWindowsWMClassName, Qt::CaseInsensitive) == 0;
                });
                sortServicesByMenuId(services, xWindowsWMClassName);
            }

            if (services.isEmpty()) {
                services = KApplicationTrader::query([&appId](const KService::Ptr &service) {
                    return service->property<QString>(QStringLiteral("StartupWMClass")).compare(appId, Qt::CaseInsensitive) == 0;
                });
                sortServicesByMenuId(services, appId);
            }

            // The appId looks like a path.
            if (services.isEmpty() && appId.startsWith(QLatin1String("/"))) {
                // Check if it's a path to a .desktop file.
                if (KDesktopFile::isDesktopFile(appId) && QFile::exists(appId)) {
                    return QUrl::fromLocalFile(appId);
                }

                // Check if the appId passes as a .desktop file path if we add the extension.
                const QString appIdPlusExtension(appId + QStringLiteral(".desktop"));

                if (KDesktopFile::isDesktopFile(appIdPlusExtension) && QFile::exists(appIdPlusExtension)) {
                    return QUrl::fromLocalFile(appIdPlusExtension);
                }
            }

            // Try matching appId against DesktopEntryName.
            if (services.isEmpty()) {
                services = KApplicationTrader::query([&appId](const KService::Ptr &service) {
                    return service->desktopEntryName().compare(appId, Qt::CaseInsensitive) == 0;
                });
                sortServicesByMenuId(services, appId);
            }

            // Try matching appId against 'Name'.
            // This has a shaky chance of success as appId is untranslated, but 'Name' may be localized.
            if (services.isEmpty()) {
                services = KApplicationTrader::query([&appId](const KService::Ptr &service) {
                    return !service->noDisplay() && service->name().compare(appId, Qt::CaseInsensitive) == 0;
                });
                sortServicesByMenuId(services, appId);
            }
        }

        // Ok, absolute *last* chance, try matching via pid (but only if we have not already tried this!) ...
        if (services.isEmpty() && !triedPid) {
            services = servicesFromPid(pid);
        }
    }

    // Try to improve on a possible from-binary fallback.
    // If no services were found or we got a fake-service back from getServicesViaPid()
    // we attempt to improve on this by adding a loosely matched reverse-domain-name
    // DesktopEntryName. Namely anything that is '*.appId.desktop' would qualify here.
    //
    // Illustrative example of a case where the above heuristics would fail to produce
    // a reasonable result:
    // - org.kde.dragonplayer.desktop
    // - binary is 'dragon'
    // - qapp appname and thus appId is 'dragonplayer'
    // - appId cannot directly match the desktop file because of RDN
    // - appId also cannot match the binary because of name mismatch
    // - in the following code *.appId can match org.kde.dragonplayer though
    if (!appId.isEmpty() /* BUG 472576 */ && (services.isEmpty() || services.at(0)->desktopEntryName().isEmpty())) {
        auto matchingServices = KApplicationTrader::query([&appId](const KService::Ptr &service) {
            return !service->noDisplay() && service->desktopEntryName().contains(appId, Qt::CaseInsensitive);
        });

        QMutableListIterator<KService::Ptr> it(matchingServices);
        while (it.hasNext()) {
            auto service = it.next();
            if (!service->desktopEntryName().endsWith(u'.' + appId)) {
                it.remove();
            }
        }
        // Exactly one match is expected, otherwise we discard the results as to reduce
        // the likelihood of false-positive mappings. Since we essentially eliminate the
        // uniqueness that RDN is meant to bring to the table we could potentially end
        // up with more than one match here.
        if (matchingServices.length() == 1) {
            services = matchingServices;
        }
    }

    if (!services.isEmpty()) {
        const QString &menuId = services.at(0)->menuId();

        // applications: URLs are used to refer to applications by their KService::menuId
        // (i.e. .desktop file name) rather than the absolute path to a .desktop file.
        if (!menuId.isEmpty()) {
            url.setUrl(QString(u"applications:" + menuId));
            return url;
        }

        QString path = services.at(0)->entryPath();

        if (path.isEmpty()) {
            path = services.at(0)->exec();
        }

        if (!path.isEmpty()) {
            QString query = url.query();
            url = QUrl::fromLocalFile(path);
            url.setQuery(query);
            return url;
        }
    }

    return url;
}

KService::List servicesFromPid(quint32 pid)
{
    if (pid == 0) {
        return KService::List();
    }

    // Read the BAMF_DESKTOP_FILE_HINT environment variable which contains the actual desktop file path for Snaps.
    QFile environFile(QStringLiteral("/proc/%1/environ").arg(QString::number(pid)));
    if (environFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        constexpr QByteArrayView bamfDesktopFileHint{"BAMF_DESKTOP_FILE_HINT"};
        constexpr QByteArrayView appDir{"APPDIR"};
        const QByteArray environ = environFile.readAll();
#if (defined(__GNUC__) && __GNUC__ >= 12) || !defined(__GNUC__)
        for (const QByteArrayView line : QByteArrayView(environ) | std::views::split('\0')) {
#else
        for (const QByteArrayView line : environ.split('\0')) {
#endif
            const auto equalsIdx = line.indexOf('=');
            if (equalsIdx == -1) {
                continue;
            }

            const QByteArrayView key = line.sliced(0, equalsIdx);
            if (key == bamfDesktopFileHint) {
                KService::Ptr service = KService::serviceByDesktopPath(QString::fromUtf8(line.sliced(equalsIdx + 1)));
                if (service) {
                    return {service};
                }
                break;
            } else if (key == appDir) {
                // For AppImage
                const QByteArrayView value = line.sliced(equalsIdx + 1);
                const auto desktopFileList = QDir(QString::fromUtf8(value)).entryInfoList(QStringList{QStringLiteral("*.desktop")}, QDir::Files);
                if (!desktopFileList.empty()) {
                    return {QExplicitlySharedDataPointer<KService>(new KService(desktopFileList[0].absoluteFilePath()))};
                }
                break;
            }
        }
    }

    auto proc = KProcessList::processInfo(pid);
    if (!proc.isValid()) {
        return KService::List();
    }

    const QString cmdLine = proc.command();

    if (cmdLine.isEmpty()) {
        return KService::List();
    }

    return servicesFromCmdLine(cmdLine, proc.name());
}

KService::List servicesFromCmdLine(const QString &_cmdLine, const QString &processName)
{
    QString cmdLine = _cmdLine;
    KService::List services;

    const int firstSpace = cmdLine.indexOf(u' ');
    int slash = 0;

    services = KApplicationTrader::query([&cmdLine](const KService::Ptr &service) {
        return service->exec() == cmdLine;
    });

    if (services.isEmpty()) {
        // Could not find with complete command line, so strip out the path part ...
        slash = cmdLine.lastIndexOf(u'/', firstSpace);

        if (slash > 0) {
            const QStringView midCmd = QStringView(cmdLine).mid(slash + 1);
            services = services = KApplicationTrader::query([&midCmd](const KService::Ptr &service) {
                return service->exec() == midCmd;
            });
        }
    }

    if (services.isEmpty() && firstSpace > 0) {
        // Could not find with arguments, so try without ...
        cmdLine.truncate(firstSpace);

        services = KApplicationTrader::query([&cmdLine](const KService::Ptr &service) {
            return service->exec() == cmdLine;
        });

        if (services.isEmpty()) {
            slash = cmdLine.lastIndexOf(u'/');

            if (slash > 0) {
                const QStringView midCmd = QStringView(cmdLine).mid(slash + 1);
                services = KApplicationTrader::query([&midCmd](const KService::Ptr &service) {
                    return service->exec() == midCmd;
                });
            }
        }
    }

    if (services.isEmpty()) {
        const QStringList runtimes = {u"perl"_s};

        bool ignore = runtimes.contains(cmdLine);

        if (!ignore && slash > 0) {
            ignore = runtimes.contains(cmdLine.mid(slash + 1));
        }

        if (ignore) {
            return servicesFromCmdLine(_cmdLine.mid(firstSpace + 1), processName);
        }
    }

    if (services.isEmpty() && !processName.isEmpty() && !QStandardPaths::findExecutable(cmdLine).isEmpty()) {
        // cmdLine now exists without arguments if there were any.
        services << QExplicitlySharedDataPointer<KService>(new KService(processName, cmdLine, QString()));
    }

    return services;
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
                KConfigGroup confGroup(KSharedConfig::openConfig(), u"General"_s);
                const QString preferredTerminal = confGroup.readPathEntry("TerminalApplication", QStringLiteral("konsole"));
                command = preferredTerminal + QLatin1String(" -e ") + command;
            }

            return command;
        }
    } else if (application.compare(QLatin1String("browser"), Qt::CaseInsensitive) == 0) {
        const auto service = DefaultService::browser();
        return service ? service->storageId() : DefaultService::legacyBrowserExec();
    } else if (application.compare(QLatin1String("terminal"), Qt::CaseInsensitive) == 0) {
        KConfigGroup confGroup(KSharedConfig::openConfig(), u"General"_s);

        return confGroup.readPathEntry("TerminalApplication", KService::serviceByStorageId(QStringLiteral("konsole")) ? QStringLiteral("konsole") : QString());
    } else if (application.compare(QLatin1String("filemanager"), Qt::CaseInsensitive) == 0) {
        KService::Ptr service = KApplicationTrader::preferredService(QStringLiteral("inode/directory"));

        if (service) {
            return service->storageId();
        }
    } else if (KService::Ptr service = KApplicationTrader::preferredService(application)) {
        return service->storageId();
    }

    return QLatin1String("");
}

bool launcherUrlsMatch(const QUrl &a, const QUrl &b, UrlComparisonMode mode)
{
    QUrl sanitizedA = a;
    QUrl sanitizedB = b;

    if (mode == IgnoreQueryItems) {
        sanitizedA = a.adjusted(QUrl::RemoveQuery);
        sanitizedB = b.adjusted(QUrl::RemoveQuery);
    }

    auto tryResolveToApplicationsUrl = [](const QUrl &url) -> QUrl {
        QUrl resolvedUrl = url;

        if (url.isLocalFile() && KDesktopFile::isDesktopFile(url.toLocalFile())) {
            KDesktopFile f(url.toLocalFile());

            const KService::Ptr service = KService::serviceByStorageId(f.fileName());

            // Resolve to non-absolute menuId-based URL if possible.
            if (service) {
                const QString &menuId = service->menuId();

                if (!menuId.isEmpty()) {
                    resolvedUrl = QUrl(QLatin1String("applications:") + menuId);
                    resolvedUrl.setQuery(url.query());
                }
            }
        }

        return resolvedUrl;
    };

    sanitizedA = tryResolveToApplicationsUrl(sanitizedA);
    sanitizedB = tryResolveToApplicationsUrl(sanitizedB);

    return (sanitizedA == sanitizedB);
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

void runApp(const AppData &appData, const QList<QUrl> &urls)
{
    if (appData.url.isValid()) {
        KService::Ptr service;

        // applications: URLs are used to refer to applications by their KService::menuId
        // (i.e. .desktop file name) rather than the absolute path to a .desktop file.
        if (appData.url.scheme() == QLatin1String("applications")) {
            service = KService::serviceByMenuId(appData.url.path());
        } else if (appData.url.scheme() == QLatin1String("preferred")) {
            service = KService::serviceByStorageId(defaultApplication(appData.url));
        } else {
            service = KService::serviceByDesktopPath(appData.url.toLocalFile());
        }

        if (service && service->isApplication()) {
            auto *job = new KIO::ApplicationLauncherJob(service);
            job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
            job->setUrls(urls);
            job->start();

            KActivities::ResourceInstance::notifyAccessed(QUrl(QString(u"applications:" + service->storageId())), QStringLiteral("org.kde.libtaskmanager"));
        } else {
            auto *job = new KIO::OpenUrlJob(appData.url);
            job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
            job->setRunExecutables(true);
            job->start();

            if (!appData.id.isEmpty()) {
                KActivities::ResourceInstance::notifyAccessed(QUrl(QString(u"applications:" + appData.id)), QStringLiteral("org.kde.libtaskmanager"));
            }
        }
    }
}

bool canLauchNewInstance(const AppData &appData)
{
    if (appData.url.isEmpty()) {
        return false;
    }

    QString desktopEntry = appData.id;

    // Remove suffix if necessary
    if (desktopEntry.endsWith(QLatin1String(".desktop"))) {
        desktopEntry.chop(8);
    }

    const KService::Ptr service = KService::serviceByDesktopName(desktopEntry);

    if (service) {
        if (service->noDisplay()) {
            return false;
        }

        if (service->property<bool>(QStringLiteral("SingleMainWindow"))) {
            return false;
        }

        // GNOME-specific key, for backwards compatibility with apps that haven't
        // started using the XDG "SingleMainWindow" key yet
        if (service->property<bool>(QStringLiteral("X-GNOME-SingleWindow"))) {
            return false;
        }

        // Hide our own action if there's already a "New Window" action
        const auto actions = service->actions();
        for (const KServiceAction &action : actions) {
            if (action.name().startsWith(u"new", Qt::CaseInsensitive) && action.name().endsWith(u"window", Qt::CaseInsensitive)) {
                return false;
            }

            if (action.name() == QLatin1String("WindowNew")) {
                return false;
            }
        }
    }

    return true;
}
}
