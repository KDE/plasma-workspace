/********************************************************************
Copyright 2016  Eike Hein <hein@kde.org>
Copyright 2008  Aaron J. Seigo <aseigo@kde.org>

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

#include "xwindowtasksmodel.h"
#include "tasktools.h"

#include <KConfigGroup>
#include <KDirWatch>
#include <KIconLoader>
#include <KRun>
#include <KService>
#include <KServiceTypeTrader>
#include <KSharedConfig>
#include <KStartupInfo>
#include <KSycoca>
#include <KWindowInfo>
#include <KWindowSystem>
#include <processcore/processes.h>
#include <processcore/process.h>

#include <QBuffer>
#include <QDir>
#include <QIcon>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>
#include <QX11Info>

namespace TaskManager
{

static const NET::Properties windowInfoFlags = NET::WMState | NET::XAWMState | NET::WMDesktop |
        NET::WMVisibleName | NET::WMGeometry | NET::WMFrameExtents |
        NET::WMWindowType;
static const NET::Properties2 windowInfoFlags2 = NET::WM2WindowClass | NET::WM2AllowedActions;

class XWindowTasksModel::Private
{
public:
    Private(XWindowTasksModel *q);
    ~Private();

    QVector<WId> windows;
    QSet<WId> transients;
    QMultiHash<WId, WId> transientsDemandingAttention;
    QHash<WId, KWindowInfo*> windowInfoCache;
    QHash<WId, AppData> appDataCache;
    QHash<WId, QRect> delegateGeometries;
    WId activeWindow = -1;
    KSharedConfig::Ptr rulesConfig;
    KDirWatch *configWatcher = nullptr;
    QTimer sycocaChangeTimer;

    void init();
    void addWindow(WId window);
    void removeWindow(WId window);
    void windowChanged(WId window, NET::Properties properties, NET::Properties2 properties2);
    void transientChanged(WId window, NET::Properties properties, NET::Properties2 properties2);
    void dataChanged(WId window, const QVector<int> &roles);

    KWindowInfo* windowInfo(WId window);
    AppData appData(WId window);

    QIcon icon(WId window);
    static QString mimeType();
    static QString groupMimeType();
    QUrl windowUrl(WId window);
    QUrl launcherUrl(WId window, bool encodeFallbackIcon = true);
    QUrl serviceUrl(int pid, const QString &type, const QStringList &cmdRemovals);
    KService::List servicesFromPid(int pid);
    QStringList activities(WId window);
    bool demandsAttention(WId window);

private:
    XWindowTasksModel *q;
};

XWindowTasksModel::Private::Private(XWindowTasksModel *q)
    : q(q)
{
}

XWindowTasksModel::Private::~Private()
{
    qDeleteAll(windowInfoCache);
    windowInfoCache.clear();
}

void XWindowTasksModel::Private::init()
{
    rulesConfig = KSharedConfig::openConfig(QStringLiteral("taskmanagerrulesrc"));
    configWatcher = new KDirWatch(q);

    foreach (const QString &location, QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)) {
        configWatcher->addFile(location + QLatin1String("/taskmanagerrulesrc"));
    }

    QObject::connect(configWatcher, &KDirWatch::dirty, [this] { rulesConfig->reparseConfiguration(); });
    QObject::connect(configWatcher, &KDirWatch::created, [this] { rulesConfig->reparseConfiguration(); });
    QObject::connect(configWatcher, &KDirWatch::deleted, [this] { rulesConfig->reparseConfiguration(); });

    sycocaChangeTimer.setSingleShot(true);
    sycocaChangeTimer.setInterval(100);

    QObject::connect(&sycocaChangeTimer, &QTimer::timeout, q,
        [this]() {
            if (!windows.count()) {
                return;
            }

            appDataCache.clear();

            // Emit changes of all roles satisfied from app data cache.
            q->dataChanged(q->index(0, 0),  q->index(windows.count() - 1, 0),
                QVector<int>{Qt::DecorationRole, AbstractTasksModel::AppId,
                AbstractTasksModel::AppName, AbstractTasksModel::GenericName,
                AbstractTasksModel::LauncherUrl});
        }
    );

    void (KSycoca::*myDatabaseChangeSignal)(const QStringList &) = &KSycoca::databaseChanged;
    QObject::connect(KSycoca::self(), myDatabaseChangeSignal, q,
        [this](const QStringList &changedResources) {
            if (changedResources.contains(QLatin1String("services"))
                || changedResources.contains(QLatin1String("apps"))
                || changedResources.contains(QLatin1String("xdgdata-apps"))) {
                sycocaChangeTimer.start();
            }
        }
    );

    QObject::connect(KWindowSystem::self(), &KWindowSystem::windowAdded, q,
        [this](WId window) {
            addWindow(window);
        }
    );

    QObject::connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, q,
        [this](WId window) {
            removeWindow(window);
        }
    );

    void (KWindowSystem::*myWindowChangeSignal)(WId window,
        NET::Properties properties, NET::Properties2 properties2) = &KWindowSystem::windowChanged;
    QObject::connect(KWindowSystem::self(), myWindowChangeSignal, q,
        [this](WId window, NET::Properties properties, NET::Properties2 properties2) {
            windowChanged(window, properties, properties2);
        }
    );

    // Update IsActive for previously- and newly-active windows.
    QObject::connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, q,
        [this](WId window) {
            const WId oldActiveWindow = activeWindow;
            activeWindow = window;

            int row = windows.indexOf(oldActiveWindow);

            if (row != -1) {
                dataChanged(oldActiveWindow, QVector<int>{IsActive});
            }

            row = windows.indexOf(window);

            if (row != -1) {
                dataChanged(window, QVector<int>{IsActive});
            }
        }
    );

    activeWindow = KWindowSystem::activeWindow();

    // Add existing windows.
    foreach(const WId window, KWindowSystem::windows()) {
        addWindow(window);
    }
}

void XWindowTasksModel::Private::addWindow(WId window)
{
    // Don't add window twice.
    if (windows.contains(window)) {
        return;
    }

    KWindowInfo info(window,
                     NET::WMWindowType | NET::WMState | NET::WMName | NET::WMVisibleName,
                     NET::WM2TransientFor);

    NET::WindowType wType = info.windowType(NET::NormalMask | NET::DesktopMask | NET::DockMask |
                                            NET::ToolbarMask | NET::MenuMask | NET::DialogMask |
                                            NET::OverrideMask | NET::TopMenuMask |
                                            NET::UtilityMask | NET::SplashMask);

    const WId leader = info.transientFor();

    // Handle transient.
    if (leader > 0 && leader != window && leader != QX11Info::appRootWindow()
        && !transients.contains(window) && windows.contains(leader)) {
        transients.insert(window);

        // Update demands attention state for leader.
        if (info.hasState(NET::DemandsAttention) && windows.contains(leader)) {
            transientsDemandingAttention.insertMulti(leader, window);
            dataChanged(leader, QVector<int>{IsDemandingAttention});
        }

        return;
    }

    // Ignore NET::Tool and other special window types; they are not considered tasks.
    if (wType != NET::Normal && wType != NET::Override && wType != NET::Unknown &&
        wType != NET::Dialog && wType != NET::Utility) {

        return;
    }

    const int count = windows.count();
    q->beginInsertRows(QModelIndex(), count, count);
    windows.append(window);
    q->endInsertRows();
}

void XWindowTasksModel::Private::removeWindow(WId window)
{
    const int row = windows.indexOf(window);

    if (row != -1) {
        q->beginRemoveRows(QModelIndex(), row, row);
        windows.removeAt(row);
        transientsDemandingAttention.remove(window);
        windowInfoCache.remove(window);
        appDataCache.remove(window);
        delegateGeometries.remove(window);
        q->endRemoveRows();
    } else { // Could be a transient.
        // Removing a transient might change the demands attention state of the leader.
        if (transients.remove(window)) {
            const WId leader = transientsDemandingAttention.key(window, XCB_WINDOW_NONE);

            if (leader != XCB_WINDOW_NONE) {
                transientsDemandingAttention.remove(leader, window);
                dataChanged(leader, QVector<int>{IsDemandingAttention});
            }
        }
    }

    if (activeWindow == window) {
        activeWindow = -1;
    }
}

void XWindowTasksModel::Private::transientChanged(WId window, NET::Properties properties, NET::Properties2 properties2)
{
    // Changes to a transient's state might change demands attention state for leader.
    if (properties & (NET::WMState | NET::XAWMState)) {
        const KWindowInfo info(window, NET::WMState | NET::XAWMState, NET::WM2TransientFor);
        const WId leader = info.transientFor();

        if (!windows.contains(leader)) {
            return;
        }

        if (info.hasState(NET::DemandsAttention)) {
            if (!transientsDemandingAttention.values(leader).contains(window)) {
                transientsDemandingAttention.insertMulti(leader, window);
                dataChanged(leader, QVector<int>{IsDemandingAttention});
            }
        } else if (transientsDemandingAttention.remove(window)) {
            dataChanged(leader, QVector<int>{IsDemandingAttention});
        }
    // Leader might have changed.
    } else if (properties2 & NET::WM2TransientFor) {
        const KWindowInfo info(window, NET::WMState | NET::XAWMState, NET::WM2TransientFor);

        if (info.hasState(NET::DemandsAttention)) {
            const WId oldLeader = transientsDemandingAttention.key(window, XCB_WINDOW_NONE);

            if (oldLeader != XCB_WINDOW_NONE) {
                const WId leader = info.transientFor();

                if (leader != oldLeader) {
                    transientsDemandingAttention.remove(oldLeader, window);
                    transientsDemandingAttention.insertMulti(leader, window);
                    dataChanged(oldLeader, QVector<int>{IsDemandingAttention});
                    dataChanged(leader, QVector<int>{IsDemandingAttention});
                }
            }
        }
    }
}

void XWindowTasksModel::Private::windowChanged(WId window, NET::Properties properties, NET::Properties2 properties2)
{
    if (transients.contains(window)) {
        transientChanged(window, properties, properties2);

        return;
    }

    bool wipeInfoCache = false;
    bool wipeAppDataCache = false;
    QVector<int> changedRoles;

    if (properties & (NET::WMName | NET::WMVisibleName | NET::WM2WindowClass | NET::WMPid)
        || properties2 & NET::WM2WindowClass) {
        wipeInfoCache = true;
        wipeAppDataCache = true;
        changedRoles << Qt::DisplayRole << Qt::DecorationRole << AppId << AppName << GenericName << LauncherUrl;
    }

    if ((properties & NET::WMIcon) && !changedRoles.contains(Qt::DecorationRole)) {
        changedRoles << Qt::DecorationRole;
    }

    // FIXME TODO: It might be worth keeping track of which windows were demanding
    // attention (or not) to avoid emitting this role on every state change, as
    // TaskGroupingProxyModel needs to check for group-ability when a change to it
    // is announced and the queried state is false.
    if (properties & (NET::WMState | NET::XAWMState)) {
        wipeInfoCache = true;
        changedRoles << IsFullScreen << IsMaximized << IsMinimized << IsKeepAbove << IsKeepBelow;
        changedRoles << IsShaded << IsDemandingAttention << SkipTaskbar << SkipPager;
    }

    if (properties & NET::WMWindowType) {
        wipeInfoCache = true;
        changedRoles << SkipTaskbar;
    }

    if (properties2 & NET::WM2AllowedActions) {
        wipeInfoCache = true;
        changedRoles << IsClosable << IsMovable << IsResizable << IsMaximizable << IsMinimizable;
        changedRoles << IsFullScreenable << IsShadeable << IsVirtualDesktopChangeable;
    }

    if (properties & NET::WMDesktop) {
        wipeInfoCache = true;
        changedRoles << VirtualDesktop << IsOnAllVirtualDesktops;
    }

    if (properties & NET::WMGeometry) {
        wipeInfoCache = true;
        changedRoles << Geometry << ScreenGeometry;
    }

    if (properties2 & NET::WM2Activities) {
        changedRoles << Activities;
    }

    if (wipeInfoCache) {
        delete windowInfoCache.take(window);
    }

    if (wipeAppDataCache) {
        appDataCache.remove(window);
    }

    if (!changedRoles.isEmpty()) {
        dataChanged(window, changedRoles);
    }
}

void XWindowTasksModel::Private::dataChanged(WId window, const QVector<int> &roles)
{
    const int i = windows.indexOf(window);

    if (i == -1) {
        return;
    }

    QModelIndex idx = q->index(i);
    emit q->dataChanged(idx, idx, roles);
}

KWindowInfo* XWindowTasksModel::Private::windowInfo(WId window)
{
    if (!windowInfoCache.contains(window)) {
        KWindowInfo *info = new KWindowInfo(window, windowInfoFlags, windowInfoFlags2);
        windowInfoCache.insert(window, info);

        return info;
    }

    return windowInfoCache.value(window);
}

AppData XWindowTasksModel::Private::appData(WId window)
{
    if (!appDataCache.contains(window)) {
        const AppData &data = appDataFromUrl(windowUrl(window));

        // If we weren't able to derive a launcher URL from the window meta data,
        // fall back to WM_CLASS Class string as app id. This helps with apps we
        // can't map to an URL due to existing outside the regular system
        // environment, e.g. wine clients.
        if (data.id.isEmpty() && data.url.isEmpty()) {
            AppData dataCopy = data;

            dataCopy.id = windowInfo(window)->windowClassClass();

            appDataCache.insert(window, dataCopy);

            return dataCopy;
        }

        appDataCache.insert(window, data);

        return data;
    }

    return appDataCache.value(window);
}

QIcon XWindowTasksModel::Private::icon(WId window)
{
    const AppData &app = appData(window);

    if (!app.icon.isNull()) {
        return app.icon;
    }

    QIcon icon;

    icon.addPixmap(KWindowSystem::icon(window, KIconLoader::SizeSmall, KIconLoader::SizeSmall, false));
    icon.addPixmap(KWindowSystem::icon(window, KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium, false));
    icon.addPixmap(KWindowSystem::icon(window, KIconLoader::SizeMedium, KIconLoader::SizeMedium, false));
    icon.addPixmap(KWindowSystem::icon(window, KIconLoader::SizeLarge, KIconLoader::SizeLarge, false));

    return icon;
}

QString XWindowTasksModel::Private::mimeType()
{
    return QStringLiteral("windowsystem/winid");
}

QString XWindowTasksModel::Private::groupMimeType()
{
    return QStringLiteral("windowsystem/multiple-winids");
}

QUrl XWindowTasksModel::Private::windowUrl(WId window)
{
    QUrl url;

    const KWindowInfo *info = windowInfo(window);
    const QString &classClass = info->windowClassClass();
    const QString &className = info->windowClassName();

    KService::List services;
    bool triedPid = false;

    if (!(classClass.isEmpty() && className.isEmpty())) {
        int pid = NETWinInfo(QX11Info::connection(), window, QX11Info::appRootWindow(), NET::WMPid, 0).pid();

        // For KCModules, if we matched on window class, etc, we would end up matching
        // to kcmshell5 itself - but we are more than likely interested in the actual
        // control module. Therefore we obtain this via the commandline. This commandline
        // may contain "kdeinit4:" or "[kdeinit]", so we remove these first.
        if (classClass == "kcmshell5") {
            url = serviceUrl(pid, QStringLiteral("KCModule"), QStringList() << QStringLiteral("kdeinit5:") << QStringLiteral("[kdeinit]"));

            if (!url.isEmpty()) {
                return url;
            }
        }

        // Check to see if this wmClass matched a saved one ...
        KConfigGroup grp(rulesConfig, "Mapping");
        KConfigGroup set(rulesConfig, "Settings");

        // Some apps have different launchers depending upon command line ...
        QStringList matchCommandLineFirst = set.readEntry("MatchCommandLineFirst", QStringList());

        if (!classClass.isEmpty() && matchCommandLineFirst.contains(classClass)) {
            triedPid = true;
            services = servicesFromPid(pid);
        }

        // Try to match using className also.
        if (!className.isEmpty() && matchCommandLineFirst.contains("::"+className)) {
            triedPid = true;
            services = servicesFromPid(pid);
        }

        // If the user has manually set a mapping, respect this first...
        QString mapped(grp.readEntry(classClass + "::" + className, QString()));

        if (mapped.endsWith(QLatin1String(".desktop"))) {
            url = QUrl(mapped);
            return url;
        }

        if (!classClass.isEmpty()) {
            if (mapped.isEmpty()) {
                mapped = grp.readEntry(classClass, QString());

                if (mapped.endsWith(QLatin1String(".desktop"))) {
                    url = QUrl(mapped);
                    return url;
                }
            }

            // Some apps, such as Wine, cannot use className to map to launcher name - as Wine itself is not a GUI app
            // So, Settings/ManualOnly lists window classes where the user will always have to manualy set the launcher ...
            QStringList manualOnly = set.readEntry("ManualOnly", QStringList());

            if (!classClass.isEmpty() && manualOnly.contains(classClass)) {
                return url;
            }

            KConfigGroup rewriteRulesGroup(rulesConfig, QStringLiteral("Rewrite Rules"));
            if (rewriteRulesGroup.hasGroup(classClass)) {
                KConfigGroup rewriteGroup(&rewriteRulesGroup, classClass);

                const QStringList &rules = rewriteGroup.groupList();
                for (const QString &rule : rules) {
                    KConfigGroup ruleGroup(&rewriteGroup, rule);

                    const QString propertyConfig = ruleGroup.readEntry(QStringLiteral("Property"), QString());

                    QString matchProperty;
                    if (propertyConfig == QLatin1String("ClassClass")) {
                        matchProperty = classClass;
                    } else if (propertyConfig == QLatin1String("ClassName")) {
                        matchProperty = className;
                    }

                    if (matchProperty.isEmpty()) {
                        continue;
                    }

                    const QString serviceSearchIdentifier = ruleGroup.readEntry(QStringLiteral("Identifier"), QString());
                    if (serviceSearchIdentifier.isEmpty()) {
                        continue;
                    }

                    QRegularExpression regExp(ruleGroup.readEntry(QStringLiteral("Match")));
                    const auto match = regExp.match(matchProperty);

                    if (match.hasMatch()) {
                        const QString actualMatch = match.captured(QStringLiteral("match"));
                        if (actualMatch.isEmpty()) {
                            continue;
                        }

                        const QString rewrittenString = ruleGroup.readEntry(QStringLiteral("Target")).arg(actualMatch);

                        services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ %2)").arg(rewrittenString, serviceSearchIdentifier));

                        if (!services.isEmpty()) {
                            break;
                        }
                    }
                }
            }

            if (!mapped.isEmpty() && services.empty()) {
                services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ DesktopEntryName)").arg(mapped));
            }

            if (!mapped.isEmpty() && services.empty()) {
                services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ Name) and (not exist NoDisplay or not NoDisplay)").arg(mapped));
            }

            // To match other docks (docky, unity, etc.) attempt to match on DesktopEntryName first ...
            if (services.empty()) {
                services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ DesktopEntryName)").arg(classClass));
            }

            // Try StartupWMClass.
            if (services.empty()) {
                services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ StartupWMClass)").arg(classClass));
            }

            // Try 'Name' - unfortunately this can be translated, so has a good chance of failing! (As it does for KDE's own "System Settings" (even in English!!))
            if (services.empty()) {
                services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ Name) and (not exist NoDisplay or not NoDisplay)").arg(classClass));
            }
        }

        // Ok, absolute *last* chance, try matching via pid (but only if we have not already tried this!) ...
        if (services.empty() && !triedPid) {
            services = servicesFromPid(pid);
        }
    }

    // Try to improve on a possible from-binary fallback.
    // If no services were found or we got a fake-service back from getServicesViaPid()
    // we attempt to improve on this by adding a loosely matched reverse-domain-name
    // DesktopEntryName. Namely anything that is '*.classClass.desktop' would qualify here.
    //
    // Illustrative example of a case where the above heuristics would fail to produce
    // a reasonable result:
    // - org.kde.dragonplayer.desktop
    // - binary is 'dragon'
    // - qapp appname and thus classClass is 'dragonplayer'
    // - classClass cannot directly match the desktop file because of RDN
    // - classClass also cannot match the binary because of name mismatch
    // - in the following code *.classClass can match org.kde.dragonplayer though
    if (services.empty() || services.at(0)->desktopEntryName().isEmpty()) {
        auto matchingServices = KServiceTypeTrader::self()->query(QStringLiteral("Application"),
            QStringLiteral("exist Exec and ('%1' ~~ DesktopEntryName)").arg(classClass));
        QMutableListIterator<KService::Ptr> it(matchingServices);
        while (it.hasNext()) {
            auto service = it.next();
            if (!service->desktopEntryName().endsWith("." + classClass)) {
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

    if (!services.empty()) {
        QString path = services[0]->entryPath();
        if (path.isEmpty()) {
            path = services[0]->exec();
        }

        if (!path.isEmpty()) {
            url = QUrl::fromLocalFile(path);
        }
    }

    return url;
}

QUrl XWindowTasksModel::Private::launcherUrl(WId window, bool encodeFallbackIcon)
{
    const AppData &data = appData(window);

    if (!encodeFallbackIcon || !data.icon.name().isEmpty()) {
        return data.url;
    }

    QUrl url = data.url;

    // Forego adding the window icon pixmap if the URL is otherwise empty.
    if (!url.isValid()) {
        return QUrl();
    }

    const QPixmap pixmap = KWindowSystem::icon(window, KIconLoader::SizeLarge, KIconLoader::SizeLarge, false);
    if (pixmap.isNull()) {
        return data.url;
    }
    QUrlQuery uQuery(url);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    uQuery.addQueryItem(QStringLiteral("iconData"), bytes.toBase64(QByteArray::Base64UrlEncoding));

    url.setQuery(uQuery);

    return url;
}

QUrl XWindowTasksModel::Private::serviceUrl(int pid, const QString &type, const QStringList &cmdRemovals = QStringList())
{
    if (pid == 0) {
        return QUrl();
    }

    KSysGuard::Processes procs;
    procs.updateOrAddProcess(pid);

    KSysGuard::Process *proc = procs.getProcess(pid);
    QString cmdline = proc ? proc->command().simplified() : QString(); // proc->command has a trailing space???

    if (cmdline.isEmpty()) {
        return QUrl();
    }

    foreach (const QString & r, cmdRemovals) {
        cmdline.replace(r, QLatin1String(""));
    }

    KService::List services = KServiceTypeTrader::self()->query(type, QStringLiteral("exist Exec and ('%1' =~ Exec)").arg(cmdline));

    if (services.empty()) {
        // Could not find with complete command line, so strip out path part ...
        int slash = cmdline.lastIndexOf('/', cmdline.indexOf(' '));
        if (slash > 0) {
            services = KServiceTypeTrader::self()->query(type, QStringLiteral("exist Exec and ('%1' =~ Exec)").arg(cmdline.mid(slash + 1)));
        }

        if (services.empty()) {
            return QUrl();
        }
    }

    if (!services.isEmpty()) {
        QString path = services[0]->entryPath();

        if (!QDir::isAbsolutePath(path)) {
            QString absolutePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kservices5/"+path);
            if (!absolutePath.isEmpty())
                path = absolutePath;
        }

        if (QFile::exists(path)) {
            return QUrl::fromLocalFile(path);
        }
    }

    return QUrl();
}

KService::List XWindowTasksModel::Private::servicesFromPid(int pid)
{
    // Attempt to find using commandline...
    KService::List services;

    if (pid == 0) {
        return services;
    }

    KSysGuard::Processes procs;
    procs.updateOrAddProcess(pid);

    KSysGuard::Process *proc = procs.getProcess(pid);
    QString cmdline = proc ? proc->command().simplified() : QString(); // proc->command has a trailing space???

    if (cmdline.isEmpty()) {
        return services;
    }

    const int firstSpace = cmdline.indexOf(' ');

    services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ Exec)").arg(cmdline));
    if (services.empty()) {
        // Could not find with complete commandline, so strip out path part...
        int slash = cmdline.lastIndexOf('/', firstSpace);
        if (slash > 0) {
            services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ Exec)").arg(cmdline.mid(slash + 1)));
        }
    }

    if (services.empty() && firstSpace > 0) {
        // Could not find with arguments, so try without...
        cmdline = cmdline.left(firstSpace);
        services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ Exec)").arg(cmdline));

        int slash = cmdline.lastIndexOf('/');
        if (slash > 0) {
            services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and ('%1' =~ Exec)").arg(cmdline.mid(slash + 1)));
        }
    }

    if (services.empty() && proc && !QStandardPaths::findExecutable(cmdline).isEmpty()) {
        // cmdline now exists without arguments if there were any
        services << QExplicitlySharedDataPointer<KService>(new KService(proc->name(), cmdline, QString()));
    }
    return services;
}

QStringList XWindowTasksModel::Private::activities(WId window)
{
    NETWinInfo ni(QX11Info::connection(), window, QX11Info::appRootWindow(), 0, NET::WM2Activities);

    const QString result(ni.activities());

    if (!result.isEmpty() && result != QLatin1String("00000000-0000-0000-0000-000000000000")) {
        return result.split(',');
    }

    return QStringList();
}

bool XWindowTasksModel::Private::demandsAttention(WId window)
{
    if (windows.contains(window)) {
        return ((windowInfo(window)->hasState(NET::DemandsAttention))
        || transientsDemandingAttention.contains(window));
    }

    return false;
}

XWindowTasksModel::XWindowTasksModel(QObject *parent)
    : AbstractWindowTasksModel(parent)
    , d(new Private(this))
{
    d->init();
}

XWindowTasksModel::~XWindowTasksModel()
{
}

QVariant XWindowTasksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()  || index.row() >= d->windows.count()) {
        return QVariant();
    }

    const WId window = d->windows.at(index.row());

    if (role == Qt::DisplayRole) {
        return d->windowInfo(window)->visibleName();
    } else if (role == Qt::DecorationRole) {
        return d->icon(window);
    } else if (role == AppId) {
        return d->appData(window).id;
    } else if (role == AppName) {
        return d->appData(window).name;
    } else if (role == GenericName) {
        return d->appData(window).genericName;
    } else if (role == LauncherUrl) {
        return d->launcherUrl(window);
    } else if (role == LauncherUrlWithoutIcon) {
        return d->launcherUrl(window, false /* encodeFallbackIcon */);
    } else if (role == LegacyWinIdList) {
        return QVariantList() << window;
    } else if (role == MimeType) {
        return d->mimeType();
    } else if (role == MimeData) {
        return QByteArray((char*)&window, sizeof(window));;
    } else if (role == IsWindow) {
        return true;
    } else if (role == IsActive) {
        return (window == d->activeWindow);
    } else if (role == IsClosable) {
        return d->windowInfo(window)->actionSupported(NET::ActionClose);
    } else if (role == IsMovable) {
        return d->windowInfo(window)->actionSupported(NET::ActionMove);
    } else if (role == IsResizable) {
        return d->windowInfo(window)->actionSupported(NET::ActionResize);
    } else if (role == IsMaximizable) {
        return d->windowInfo(window)->actionSupported(NET::ActionMax);
    } else if (role == IsMaximized) {
        const KWindowInfo *info = d->windowInfo(window);
        return info->hasState(NET::MaxHoriz) && info->hasState(NET::MaxVert);
    } else if (role == IsMinimizable) {
        return d->windowInfo(window)->actionSupported(NET::ActionMinimize);
    } else if (role == IsMinimized) {
        return d->windowInfo(window)->isMinimized();
    } else if (role == IsKeepAbove) {
        return d->windowInfo(window)->hasState(NET::StaysOnTop);
    } else if (role == IsKeepBelow) {
        return d->windowInfo(window)->hasState(NET::KeepBelow);
    } else if (role == IsFullScreenable) {
        return d->windowInfo(window)->actionSupported(NET::ActionFullScreen);
    } else if (role == IsFullScreen) {
        return d->windowInfo(window)->hasState(NET::FullScreen);
    } else if (role == IsShadeable) {
        return d->windowInfo(window)->actionSupported(NET::ActionShade);
    } else if (role == IsShaded) {
        return d->windowInfo(window)->hasState(NET::Shaded);
    } else if (role == IsVirtualDesktopChangeable) {
        return d->windowInfo(window)->actionSupported(NET::ActionChangeDesktop);
    } else if (role == VirtualDesktop) {
        return d->windowInfo(window)->desktop();
    } else if (role == IsOnAllVirtualDesktops) {
        return d->windowInfo(window)->onAllDesktops();
    } else if (role == Geometry) {
        return d->windowInfo(window)->frameGeometry();
    } else if (role == ScreenGeometry) {
        return screenGeometry(d->windowInfo(window)->frameGeometry().center());
    } else if (role == Activities) {
        return d->activities(window);
    } else if (role == IsDemandingAttention) {
        return d->demandsAttention(window);
    } else if (role == SkipTaskbar) {
        const KWindowInfo *info = d->windowInfo(window);
        // _NET_WM_WINDOW_TYPE_UTILITY type windows should not be on task bars,
        // but they should be shown on pagers.
        return (info->hasState(NET::SkipTaskbar) || info->windowType(NET::UtilityMask) == NET::Utility);
    } else if (role == SkipPager) {
        return d->windowInfo(window)->hasState(NET::SkipPager);
    }

    return QVariant();
}

int XWindowTasksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->windows.count();
}

void XWindowTasksModel::requestActivate(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    if (index.row() >= 0 && index.row() < d->windows.count()) {
        WId window = d->windows.at(index.row());

        // Pull forward any transient demanding attention.
        if (d->transientsDemandingAttention.contains(window)) {
            window = d->transientsDemandingAttention.value(window);
        // Quote from legacy libtaskmanager:
        // "this is a work around for (at least?) kwin where a shaded transient will prevent the main
        // window from being brought forward unless the transient is actually pulled forward, most
        // easily reproduced by opening a modal file open/save dialog on an app then shading the file
        // dialog and trying to bring the window forward by clicking on it in a tasks widget
        // TODO: do we need to check all the transients for shaded?"
        } else if (!d->transients.isEmpty()) {
            foreach (const WId transient, d->transients) {
                KWindowInfo info(transient, NET::WMState, NET::WM2TransientFor);

                if (info.valid(true) && info.hasState(NET::Shaded) && info.transientFor() == window) {
                    window = transient;
                    break;
                }
            }
        }

        KWindowSystem::forceActiveWindow(window);
    }
}

void XWindowTasksModel::requestNewInstance(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const QUrl &url = d->appData(d->windows.at(index.row())).url;

    if (url.isValid()) {
        new KRun(url, 0, false, KStartupInfo::createNewStartupIdForTimestamp(QX11Info::appUserTime()));
    }
}

void XWindowTasksModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    if (!index.isValid() || index.model() != this || index.row() < 0
        || index.row() >= d->windows.count()
        || urls.isEmpty()) {
        return;
    }

    const QUrl &url = d->appData(d->windows.at(index.row())).url;
    const KService::Ptr service = KService::serviceByDesktopPath(url.toLocalFile());
    if (service) {
        KRun::runApplication(*service, urls, nullptr, 0, {}, KStartupInfo::createNewStartupIdForTimestamp(QX11Info::appUserTime()));
    }
}

void XWindowTasksModel::requestClose(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    NETRootInfo ri(QX11Info::connection(), NET::CloseWindow);
    ri.closeWindowRequest(d->windows.at(index.row()));
}

void XWindowTasksModel::requestMove(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    bool onCurrent = info->isOnCurrentDesktop();

    if (!onCurrent) {
        KWindowSystem::setCurrentDesktop(info->desktop());
        KWindowSystem::forceActiveWindow(window);
    }

    if (info->isMinimized()) {
        KWindowSystem::unminimizeWindow(window);
    }

    const QRect &geom = info->geometry();

    NETRootInfo ri(QX11Info::connection(), NET::WMMoveResize);
    ri.moveResizeRequest(window, geom.center().x(), geom.center().y(), NET::Move);
}

void XWindowTasksModel::requestResize(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    bool onCurrent = info->isOnCurrentDesktop();

    if (!onCurrent) {
        KWindowSystem::setCurrentDesktop(info->desktop());
        KWindowSystem::forceActiveWindow(window);
    }

    if (info->isMinimized()) {
        KWindowSystem::unminimizeWindow(window);
    }

    const QRect &geom = info->geometry();

    NETRootInfo ri(QX11Info::connection(), NET::WMMoveResize);
    ri.moveResizeRequest(window, geom.bottomRight().x(), geom.bottomRight().y(), NET::BottomRight);
}

void XWindowTasksModel::requestToggleMinimized(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    if (info->isMinimized()) {
        bool onCurrent = info->isOnCurrentDesktop();

        // FIXME: Move logic up into proxy? (See also others.)
        if (!onCurrent) {
            KWindowSystem::setCurrentDesktop(info->desktop());
        }

        KWindowSystem::unminimizeWindow(window);

        if (onCurrent) {
            KWindowSystem::forceActiveWindow(window);
        }
    } else {
        KWindowSystem::minimizeWindow(window);
    }
}

void XWindowTasksModel::requestToggleMaximized(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);
    bool onCurrent = info->isOnCurrentDesktop();
    bool restore = (info->hasState(NET::MaxHoriz) && info->hasState(NET::MaxVert));

    // FIXME: Move logic up into proxy? (See also others.)
    if (!onCurrent) {
        KWindowSystem::setCurrentDesktop(info->desktop());
    }

    if (info->isMinimized()) {
        KWindowSystem::unminimizeWindow(window);
    }

    NETWinInfo ni(QX11Info::connection(), window, QX11Info::appRootWindow(), NET::WMState, 0);

    if (restore) {
        ni.setState(0, NET::Max);
    } else {
        ni.setState(NET::Max, NET::Max);
    }

    if (!onCurrent) {
        KWindowSystem::forceActiveWindow(window);
    }
}

void XWindowTasksModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    NETWinInfo ni(QX11Info::connection(), window, QX11Info::appRootWindow(), NET::WMState, 0);

    if (info->hasState(NET::StaysOnTop)) {
        ni.setState(0, NET::StaysOnTop);
    } else {
        ni.setState(NET::StaysOnTop, NET::StaysOnTop);
    }
}

void XWindowTasksModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    NETWinInfo ni(QX11Info::connection(), window, QX11Info::appRootWindow(), NET::WMState, 0);

    if (info->hasState(NET::KeepBelow)) {
        ni.setState(0, NET::KeepBelow);
    } else {
        ni.setState(NET::KeepBelow, NET::KeepBelow);
    }
}

void XWindowTasksModel::requestToggleFullScreen(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    NETWinInfo ni(QX11Info::connection(), window, QX11Info::appRootWindow(), NET::WMState, 0);

    if (info->hasState(NET::FullScreen)) {
        ni.setState(0, NET::FullScreen);
    } else {
        ni.setState(NET::FullScreen, NET::FullScreen);
    }
}

void XWindowTasksModel::requestToggleShaded(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    NETWinInfo ni(QX11Info::connection(), window, QX11Info::appRootWindow(), NET::WMState, 0);

    if (info->hasState(NET::Shaded)) {
        ni.setState(0, NET::Shaded);
    } else {
        ni.setState(NET::Shaded, NET::Shaded);
    }
}

void XWindowTasksModel::requestVirtualDesktop(const QModelIndex &index, qint32 desktop)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    if (desktop == 0) {
        if (info->onAllDesktops()) {
            KWindowSystem::setOnDesktop(window, KWindowSystem::currentDesktop());
            KWindowSystem::forceActiveWindow(window);
        } else {
            KWindowSystem::setOnAllDesktops(window, true);
        }

        return;
    // FIXME Move add-new-desktop logic up into proxy.
    } else if (desktop > KWindowSystem::numberOfDesktops()) {
        desktop = KWindowSystem::numberOfDesktops() + 1;

        // FIXME Arbitrary limit of 20 copied from old code.
        if (desktop > 20) {
            return;
        }

        NETRootInfo ri(QX11Info::connection(), NET::NumberOfDesktops);
        ri.setNumberOfDesktops(desktop);
    }

    KWindowSystem::setOnDesktop(window, desktop);

    if (desktop == KWindowSystem::currentDesktop()) {
        KWindowSystem::forceActiveWindow(window);
    }
}

void XWindowTasksModel::requestActivities(const QModelIndex &index, const QStringList &activities)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());

    KWindowSystem::setOnActivities(window, activities);
}


void XWindowTasksModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    Q_UNUSED(delegate)

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());

    if (d->delegateGeometries.contains(window)
        && d->delegateGeometries.value(window) == geometry) {
        return;
    }

    NETWinInfo ni(QX11Info::connection(), window, QX11Info::appRootWindow(), 0, 0);
    NETRect rect;

    if (geometry.isValid()) {
        rect.pos.x = geometry.x();
        rect.pos.y = geometry.y();
        rect.size.width = geometry.width();
        rect.size.height = geometry.height();

        d->delegateGeometries.insert(window, geometry);
    } else {
        d->delegateGeometries.remove(window);
    }

    ni.setIconGeometry(rect);
}

WId XWindowTasksModel::winIdFromMimeData(const QMimeData *mimeData, bool *ok)
{
    Q_ASSERT(mimeData);

    if (ok) {
        *ok = false;
    }

    if (!mimeData->hasFormat(Private::mimeType())) {
        return 0;
    }

    QByteArray data(mimeData->data(Private::mimeType()));
    if (data.size() != sizeof(WId)) {
        return 0;
    }

    WId id;
    memcpy(&id, data.data(), sizeof(WId));

    if (ok) {
        *ok = true;
    }

    return id;
}

QList<WId> XWindowTasksModel::winIdsFromMimeData(const QMimeData *mimeData, bool *ok)
{
    Q_ASSERT(mimeData);
    QList<WId> ids;

    if (ok) {
        *ok = false;
    }

    if (!mimeData->hasFormat(Private::groupMimeType())) {
        // Try to extract single window id.
        bool singularOk;
        WId id = winIdFromMimeData(mimeData, &singularOk);

        if (ok) {
            *ok = singularOk;
        }

        if (singularOk) {
            ids << id;
        }

        return ids;
    }

    QByteArray data(mimeData->data(Private::groupMimeType()));
    if ((unsigned int)data.size() < sizeof(int) + sizeof(WId)) {
        return ids;
    }

    int count = 0;
    memcpy(&count, data.data(), sizeof(int));
    if (count < 1 || (unsigned int)data.size() < sizeof(int) + sizeof(WId) * count) {
        return ids;
    }

    WId id;
    for (int i = 0; i < count; ++i) {
        memcpy(&id, data.data() + sizeof(int) + sizeof(WId) * i, sizeof(WId));
        ids << id;
    }

    if (ok) {
        *ok = true;
    }

    return ids;
}

}
