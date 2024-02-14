/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2008 Aaron J. Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "xwindowtasksmodel.h"
#include "tasktools.h"
#include "xwindowsystemeventbatcher.h"

#include <KDesktopFile>
#include <KDirWatch>
#include <KIconLoader>
#include <KService>
#include <KSharedConfig>
#include <KSycoca>
#include <KWindowInfo>
#include <KX11Extras>

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QSet>
#include <QTimer>
#include <QUrlQuery>

#include <chrono>

#include <X11/Xlib.h>

using namespace std::chrono_literals;

namespace X11Info
{
[[nodiscard]] inline auto display()
{
    return qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->display();
}

[[nodiscard]] inline auto connection()
{
    return qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
}

[[nodiscard]] inline Window appRootWindow()
{
    return DefaultRootWindow(display());
}
}

namespace TaskManager
{
static const NET::Properties windowInfoFlags =
    NET::WMState | NET::XAWMState | NET::WMDesktop | NET::WMVisibleName | NET::WMGeometry | NET::WMFrameExtents | NET::WMWindowType | NET::WMPid;
static const NET::Properties2 windowInfoFlags2 = NET::WM2DesktopFileName | NET::WM2Activities | NET::WM2WindowClass | NET::WM2AllowedActions
    | NET::WM2AppMenuObjectPath | NET::WM2AppMenuServiceName | NET::WM2GTKApplicationId;

class Q_DECL_HIDDEN XWindowTasksModel::Private
{
public:
    Private(XWindowTasksModel *q);
    ~Private();

    QList<WId> windows;

    // key=transient child, value=leader
    QHash<WId, WId> transients;
    // key=leader, values=transient children
    QMultiHash<WId, WId> transientsDemandingAttention;

    QHash<WId, KWindowInfo *> windowInfoCache;
    QHash<WId, AppData> appDataCache;
    QHash<WId, QRect> delegateGeometries;
    QSet<WId> usingFallbackIcon;
    QHash<WId, QTime> lastActivated;
    QList<WId> cachedStackingOrder;
    WId activeWindow = -1;
    KSharedConfig::Ptr rulesConfig;
    KDirWatch *configWatcher = nullptr;
    QTimer sycocaChangeTimer;

    void init();
    void addWindow(WId window);
    void removeWindow(WId window);
    void windowChanged(WId window, NET::Properties properties, NET::Properties2 properties2);
    void transientChanged(WId window, NET::Properties properties, NET::Properties2 properties2);
    void dataChanged(WId window, const QList<int> &roles);

    KWindowInfo *windowInfo(WId window);
    const AppData &appData(WId window);
    QString appMenuServiceName(WId window);
    QString appMenuObjectPath(WId window);

    QIcon icon(WId window);
    static QString mimeType();
    static QString groupMimeType();
    QUrl windowUrl(WId window);
    QUrl launcherUrl(WId window, bool encodeFallbackIcon = true);
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
    auto clearCacheAndRefresh = [this] {
        if (!windows.count()) {
            return;
        }

        appDataCache.clear();

        // Emit changes of all roles satisfied from app data cache.
        Q_EMIT q->dataChanged(q->index(0, 0),
                              q->index(windows.count() - 1, 0),
                              QList<int>{Qt::DecorationRole,
                                         AbstractTasksModel::AppId,
                                         AbstractTasksModel::AppName,
                                         AbstractTasksModel::GenericName,
                                         AbstractTasksModel::LauncherUrl,
                                         AbstractTasksModel::LauncherUrlWithoutIcon,
                                         AbstractTasksModel::CanLaunchNewInstance,
                                         AbstractTasksModel::SkipTaskbar});
    };

    cachedStackingOrder = KX11Extras::stackingOrder();

    sycocaChangeTimer.setSingleShot(true);
    sycocaChangeTimer.setInterval(100ms);

    QObject::connect(&sycocaChangeTimer, &QTimer::timeout, q, clearCacheAndRefresh);

    QObject::connect(KSycoca::self(), &KSycoca::databaseChanged, q, [this]() {
        sycocaChangeTimer.start();
    });

    rulesConfig = KSharedConfig::openConfig(QStringLiteral("taskmanagerrulesrc"));
    configWatcher = new KDirWatch(q);

    foreach (const QString &location, QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)) {
        configWatcher->addFile(location + QLatin1String("/taskmanagerrulesrc"));
    }

    auto rulesConfigChange = [this, clearCacheAndRefresh] {
        rulesConfig->reparseConfiguration();
        clearCacheAndRefresh();
    };

    QObject::connect(configWatcher, &KDirWatch::dirty, rulesConfigChange);
    QObject::connect(configWatcher, &KDirWatch::created, rulesConfigChange);
    QObject::connect(configWatcher, &KDirWatch::deleted, rulesConfigChange);

    auto windowSystem = new XWindowSystemEventBatcher(q);

    QObject::connect(windowSystem, &XWindowSystemEventBatcher::windowAdded, q, [this](WId window) {
        addWindow(window);
    });

    QObject::connect(windowSystem, &XWindowSystemEventBatcher::windowRemoved, q, [this](WId window) {
        removeWindow(window);
    });

    QObject::connect(windowSystem, &XWindowSystemEventBatcher::windowChanged, q, [this](WId window, NET::Properties properties, NET::Properties2 properties2) {
        windowChanged(window, properties, properties2);
    });

    // Update IsActive for previously- and newly-active windows.
    QObject::connect(KX11Extras::self(), &KX11Extras::activeWindowChanged, q, [this](WId window) {
        const WId oldActiveWindow = activeWindow;

        const auto leader = transients.value(window, XCB_WINDOW_NONE);
        if (leader != XCB_WINDOW_NONE) {
            window = leader;
        }

        activeWindow = window;
        lastActivated[activeWindow] = QTime::currentTime();

        int row = windows.indexOf(oldActiveWindow);

        if (row != -1) {
            dataChanged(oldActiveWindow, QList<int>{IsActive});
        }

        row = windows.indexOf(window);

        if (row != -1) {
            dataChanged(window, QList<int>{IsActive});
        }
    });

    QObject::connect(KX11Extras::self(), &KX11Extras::stackingOrderChanged, q, [this]() {
        // No need to do anything if the model is empty. This avoids calling q->dataChanged with an invalid QModelIndex.
        if (q->rowCount() == 0) {
            return;
        }
        cachedStackingOrder = KX11Extras::stackingOrder();
        Q_ASSERT(q->hasIndex(0, 0));
        Q_ASSERT(q->hasIndex(q->rowCount() - 1, 0));
        Q_EMIT q->dataChanged(q->index(0, 0), q->index(q->rowCount() - 1, 0), QList<int>{StackingOrder});
    });

    activeWindow = KX11Extras::activeWindow();

    // Add existing windows.
    foreach (const WId window, KX11Extras::windows()) {
        addWindow(window);
    }
}

void XWindowTasksModel::Private::addWindow(WId window)
{
    // Don't add window twice.
    if (windows.contains(window)) {
        return;
    }

    KWindowInfo info(window, NET::WMWindowType | NET::WMState | NET::WMName | NET::WMVisibleName, NET::WM2TransientFor);

    NET::WindowType wType = info.windowType(NET::NormalMask | NET::DesktopMask | NET::DockMask | NET::ToolbarMask | NET::MenuMask | NET::DialogMask
                                            | NET::OverrideMask | NET::TopMenuMask | NET::UtilityMask | NET::SplashMask | NET::NotificationMask);

    const WId leader = info.transientFor();

    // Handle transient.
    if (leader > 0 && leader != window && leader != X11Info::appRootWindow() && !transients.contains(window) && windows.contains(leader)) {
        transients.insert(window, leader);

        // Update demands attention state for leader.
        if (info.hasState(NET::DemandsAttention) && windows.contains(leader)) {
            transientsDemandingAttention.insert(leader, window);
            dataChanged(leader, QList<int>{IsDemandingAttention});
        }

        return;
    }

    // Ignore NET::Tool and other special window types; they are not considered tasks.
    if (wType != NET::Normal && wType != NET::Override && wType != NET::Unknown && wType != NET::Dialog && wType != NET::Utility) {
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
        delete windowInfoCache.take(window);
        appDataCache.remove(window);
        delegateGeometries.remove(window);
        usingFallbackIcon.remove(window);
        lastActivated.remove(window);
        q->endRemoveRows();
    } else { // Could be a transient.
        // Removing a transient might change the demands attention state of the leader.
        if (transients.remove(window)) {
            const WId leader = transientsDemandingAttention.key(window, XCB_WINDOW_NONE);

            if (leader != XCB_WINDOW_NONE) {
                transientsDemandingAttention.remove(leader, window);
                dataChanged(leader, QList<int>{IsDemandingAttention});
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
                transientsDemandingAttention.insert(leader, window);
                dataChanged(leader, QList<int>{IsDemandingAttention});
            }
        } else if (transientsDemandingAttention.remove(window)) {
            dataChanged(leader, QList<int>{IsDemandingAttention});
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
                    transientsDemandingAttention.insert(leader, window);
                    dataChanged(oldLeader, QList<int>{IsDemandingAttention});
                    dataChanged(leader, QList<int>{IsDemandingAttention});
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
    QList<int> changedRoles;

    if (properties & (NET::WMPid) || properties2 & (NET::WM2DesktopFileName | NET::WM2WindowClass)) {
        wipeInfoCache = true;
        wipeAppDataCache = true;
        changedRoles << Qt::DecorationRole << AppId << AppName << GenericName << LauncherUrl << AppPid << SkipTaskbar << CanLaunchNewInstance;
    }

    if (properties & (NET::WMName | NET::WMVisibleName)) {
        changedRoles << Qt::DisplayRole;
        wipeInfoCache = true;
    }

    if ((properties & NET::WMIcon) && usingFallbackIcon.contains(window)) {
        wipeAppDataCache = true;

        if (!changedRoles.contains(Qt::DecorationRole)) {
            changedRoles << Qt::DecorationRole;
        }
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
        changedRoles << IsFullScreenable << IsShadeable << IsVirtualDesktopsChangeable;
    }

    if (properties & NET::WMDesktop) {
        wipeInfoCache = true;
        changedRoles << VirtualDesktops << IsOnAllVirtualDesktops;
    }

    if (properties & NET::WMGeometry) {
        wipeInfoCache = true;
        changedRoles << Geometry << ScreenGeometry;
    }

    if (properties2 & NET::WM2Activities) {
        wipeInfoCache = true;
        changedRoles << Activities;
    }

    if (properties2 & NET::WM2AppMenuServiceName) {
        wipeInfoCache = true;
        changedRoles << ApplicationMenuServiceName;
    }

    if (properties2 & NET::WM2AppMenuObjectPath) {
        wipeInfoCache = true;
        changedRoles << ApplicationMenuObjectPath;
    }

    if (wipeInfoCache) {
        delete windowInfoCache.take(window);
    }

    if (wipeAppDataCache) {
        appDataCache.remove(window);
        usingFallbackIcon.remove(window);
    }

    if (!changedRoles.isEmpty()) {
        dataChanged(window, changedRoles);
    }
}

void XWindowTasksModel::Private::dataChanged(WId window, const QList<int> &roles)
{
    const int i = windows.indexOf(window);

    if (i == -1) {
        return;
    }

    QModelIndex idx = q->index(i);
    Q_EMIT q->dataChanged(idx, idx, roles);
}

KWindowInfo *XWindowTasksModel::Private::windowInfo(WId window)
{
    const auto &it = windowInfoCache.constFind(window);

    if (it != windowInfoCache.constEnd()) {
        return *it;
    }

    KWindowInfo *info = new KWindowInfo(window, windowInfoFlags, windowInfoFlags2);
    windowInfoCache.insert(window, info);

    return info;
}

const AppData &XWindowTasksModel::Private::appData(WId window)
{
    static_assert(!std::is_trivially_copy_assignable_v<AppData>);
    if (auto it = appDataCache.constFind(window); it != appDataCache.constEnd()) {
        return *it;
    }

    AppData data = appDataFromUrl(windowUrl(window));

    // If we weren't able to derive a launcher URL from the window meta data,
    // fall back to WM_CLASS Class string as app id. This helps with apps we
    // can't map to an URL due to existing outside the regular system
    // environment, e.g. wine clients.
    if (data.id.isEmpty() && data.url.isEmpty()) {
        data.id = windowInfo(window)->windowClassClass();
        return *appDataCache.emplace(window, std::move(data));
    }

    return *appDataCache.emplace(window, std::move(data));
}

QString XWindowTasksModel::Private::appMenuServiceName(WId window)
{
    const KWindowInfo *info = windowInfo(window);
    return QString::fromUtf8(info->applicationMenuServiceName());
}

QString XWindowTasksModel::Private::appMenuObjectPath(WId window)
{
    const KWindowInfo *info = windowInfo(window);
    return QString::fromUtf8(info->applicationMenuObjectPath());
}

QIcon XWindowTasksModel::Private::icon(WId window)
{
    const AppData &app = appData(window);

    if (!app.icon.isNull()) {
        return app.icon;
    }

    QIcon icon;

    icon.addPixmap(KX11Extras::icon(window, KIconLoader::SizeSmall, KIconLoader::SizeSmall, false));
    icon.addPixmap(KX11Extras::icon(window, KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium, false));
    icon.addPixmap(KX11Extras::icon(window, KIconLoader::SizeMedium, KIconLoader::SizeMedium, false));
    icon.addPixmap(KX11Extras::icon(window, KIconLoader::SizeLarge, KIconLoader::SizeLarge, false));

    appDataCache[window].icon = icon;
    usingFallbackIcon.insert(window);

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
    const KWindowInfo *info = windowInfo(window);

    QString desktopFile = QString::fromUtf8(info->desktopFileName());

    if (desktopFile.isEmpty()) {
        desktopFile = QString::fromUtf8(info->gtkApplicationId());
    }

    if (!desktopFile.isEmpty()) {
        KService::Ptr service = KService::serviceByStorageId(desktopFile);

        if (service) {
            const QString &menuId = service->menuId();

            // applications: URLs are used to refer to applications by their KService::menuId
            // (i.e. .desktop file name) rather than the absolute path to a .desktop file.
            if (!menuId.isEmpty()) {
                return QUrl(QStringLiteral("applications:") + menuId);
            }

            return QUrl::fromLocalFile(service->entryPath());
        }

        if (!desktopFile.endsWith(QLatin1String(".desktop"))) {
            desktopFile.append(QLatin1String(".desktop"));
        }

        if (KDesktopFile::isDesktopFile(desktopFile) && QFile::exists(desktopFile)) {
            return QUrl::fromLocalFile(desktopFile);
        }
    }

    return windowUrlFromMetadata(info->windowClassClass(), info->pid(), rulesConfig, info->windowClassName());
}

QUrl XWindowTasksModel::Private::launcherUrl(WId window, bool encodeFallbackIcon)
{
    const AppData &data = appData(window);

    QUrl url = data.url;
    if (!encodeFallbackIcon || !data.icon.name().isEmpty()) {
        return url;
    }

    // Forego adding the window icon pixmap if the URL is otherwise empty.
    if (!url.isValid()) {
        return QUrl();
    }

    // Only serialize pixmap data if the window pixmap is actually being used.
    // QIcon::name() used above only returns a themed icon name but nothing when
    // the icon was created using an absolute path, as can be the case with, e.g.
    // containerized apps.
    if (!usingFallbackIcon.contains(window)) {
        return url;
    }

    QPixmap pixmap;

    if (!data.icon.isNull()) {
        pixmap = data.icon.pixmap(KIconLoader::SizeLarge);
    }

    if (pixmap.isNull()) {
        pixmap = KX11Extras::icon(window, KIconLoader::SizeLarge, KIconLoader::SizeLarge, false);
    }

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

bool XWindowTasksModel::Private::demandsAttention(WId window)
{
    if (windows.contains(window)) {
        return ((windowInfo(window)->hasState(NET::DemandsAttention)) || transientsDemandingAttention.contains(window));
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
    if (!index.isValid() || index.row() >= d->windows.count()) {
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
    } else if (role == WinIdList) {
        return QVariantList() << window;
    } else if (role == MimeType) {
        return d->mimeType();
    } else if (role == MimeData) {
        return QByteArray(reinterpret_cast<const char *>(&window), sizeof(window));
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
    } else if (role == IsHidden) {
        return d->windowInfo(window)->hasState(NET::Hidden);
    } else if (role == IsKeepAbove) {
        return d->windowInfo(window)->hasState(NET::KeepAbove);
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
    } else if (role == IsVirtualDesktopsChangeable) {
        return d->windowInfo(window)->actionSupported(NET::ActionChangeDesktop);
    } else if (role == VirtualDesktops) {
        return QVariantList() << d->windowInfo(window)->desktop();
    } else if (role == IsOnAllVirtualDesktops) {
        return d->windowInfo(window)->onAllDesktops();
    } else if (role == Geometry) {
        // Both the topLeft position and the size belong to the non-scaled coordinate system
        return d->windowInfo(window)->frameGeometry();
    } else if (role == ScreenGeometry) {
        // The topLeft position belongs to the non-scaled coordinate system
        // The size belongs to the scaled coordinate system, which means the size is already divided by DPR
        return screenGeometry(d->windowInfo(window)->frameGeometry().center());
    } else if (role == Activities) {
        return d->windowInfo(window)->activities();
    } else if (role == IsDemandingAttention) {
        return d->demandsAttention(window);
    } else if (role == SkipTaskbar) {
        const KWindowInfo *info = d->windowInfo(window);
        // _NET_WM_WINDOW_TYPE_UTILITY type windows should not be on task bars,
        // but they should be shown on pagers.
        return (info->hasState(NET::SkipTaskbar) || info->windowType(NET::UtilityMask) == NET::Utility || d->appData(window).skipTaskbar);
    } else if (role == SkipPager) {
        return d->windowInfo(window)->hasState(NET::SkipPager);
    } else if (role == AppPid) {
        return d->windowInfo(window)->pid();
    } else if (role == StackingOrder) {
        return d->cachedStackingOrder.indexOf(window);
    } else if (role == LastActivated) {
        if (d->lastActivated.contains(window)) {
            return d->lastActivated.value(window);
        }
    } else if (role == ApplicationMenuObjectPath) {
        return d->appMenuObjectPath(window);
    } else if (role == ApplicationMenuServiceName) {
        return d->appMenuServiceName(window);
    } else if (role == CanLaunchNewInstance) {
        return canLauchNewInstance(d->appData(window));
    }

    return AbstractTasksModel::data(index, role);
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
            const auto transients = d->transients.keys(window);
            for (const auto transient : std::as_const(transients)) {
                KWindowInfo info(transient, NET::WMState, NET::WM2TransientFor);
                if (info.valid(true) && info.hasState(NET::Shaded)) {
                    window = transient;
                    break;
                }
            }
        }

        KX11Extras::forceActiveWindow(window);
    }
}

void XWindowTasksModel::requestNewInstance(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    runApp(d->appData(d->windows.at(index.row())));
}

void XWindowTasksModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count() || urls.isEmpty()) {
        return;
    }

    runApp(d->appData(d->windows.at(index.row())), urls);
}

void XWindowTasksModel::requestClose(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    NETRootInfo ri(X11Info::connection(), NET::CloseWindow);
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
        KX11Extras::setCurrentDesktop(info->desktop());
        KX11Extras::forceActiveWindow(window);
    }

    if (info->isMinimized()) {
        KX11Extras::unminimizeWindow(window);
    }

    const QRect &geom = info->geometry();

    NETRootInfo ri(X11Info::connection(), NET::WMMoveResize);
    ri.moveResizeRequest(window, geom.center().x(), geom.center().y(), NET::Move, XCB_BUTTON_INDEX_ANY);
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
        KX11Extras::setCurrentDesktop(info->desktop());
        KX11Extras::forceActiveWindow(window);
    }

    if (info->isMinimized()) {
        KX11Extras::unminimizeWindow(window);
    }

    const QRect &geom = info->geometry();

    NETRootInfo ri(X11Info::connection(), NET::WMMoveResize);
    ri.moveResizeRequest(window, geom.bottomRight().x(), geom.bottomRight().y(), NET::BottomRight, XCB_BUTTON_INDEX_ANY);
}

void XWindowTasksModel::requestToggleMinimized(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    if (index.data(AbstractTasksModel::IsHidden).toBool()) {
        bool onCurrent = info->isOnCurrentDesktop();

        // FIXME: Move logic up into proxy? (See also others.)
        if (!onCurrent) {
            KX11Extras::setCurrentDesktop(info->desktop());
        }

        KX11Extras::unminimizeWindow(window);

        if (onCurrent) {
            KX11Extras::forceActiveWindow(window);
        }
    } else {
        KX11Extras::minimizeWindow(window);
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
        KX11Extras::setCurrentDesktop(info->desktop());
    }

    if (info->isMinimized()) {
        KX11Extras::unminimizeWindow(window);
    }

    NETWinInfo ni(X11Info::connection(), window, X11Info::appRootWindow(), NET::WMState, NET::Properties2());

    if (restore) {
        ni.setState(NET::States(), NET::Max);
    } else {
        ni.setState(NET::Max, NET::Max);
    }

    if (!onCurrent) {
        KX11Extras::forceActiveWindow(window);
    }
}

void XWindowTasksModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    NETWinInfo ni(X11Info::connection(), window, X11Info::appRootWindow(), NET::WMState, NET::Properties2());

    if (info->hasState(NET::KeepAbove)) {
        ni.setState(NET::States(), NET::KeepAbove);
    } else {
        ni.setState(NET::KeepAbove, NET::KeepAbove);
    }
}

void XWindowTasksModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    NETWinInfo ni(X11Info::connection(), window, X11Info::appRootWindow(), NET::WMState, NET::Properties2());

    if (info->hasState(NET::KeepBelow)) {
        ni.setState(NET::States(), NET::KeepBelow);
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

    NETWinInfo ni(X11Info::connection(), window, X11Info::appRootWindow(), NET::WMState, NET::Properties2());

    if (info->hasState(NET::FullScreen)) {
        ni.setState(NET::States(), NET::FullScreen);
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

    NETWinInfo ni(X11Info::connection(), window, X11Info::appRootWindow(), NET::WMState, NET::Properties2());

    if (info->hasState(NET::Shaded)) {
        ni.setState(NET::States(), NET::Shaded);
    } else {
        ni.setState(NET::Shaded, NET::Shaded);
    }
}

void XWindowTasksModel::requestVirtualDesktops(const QModelIndex &index, const QVariantList &desktops)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    int desktop = 0;

    if (!desktops.isEmpty()) {
        bool ok = false;

        desktop = desktops.first().toUInt(&ok);

        if (!ok) {
            return;
        }
    }

    if (desktop > KX11Extras::numberOfDesktops()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const KWindowInfo *info = d->windowInfo(window);

    if (desktop == 0) {
        if (info->onAllDesktops()) {
            KX11Extras::setOnDesktop(window, KX11Extras::currentDesktop());
            KX11Extras::forceActiveWindow(window);
        } else {
            KX11Extras::setOnAllDesktops(window, true);
        }

        return;
    }

    KX11Extras::setOnDesktop(window, desktop);

    if (desktop == KX11Extras::currentDesktop()) {
        KX11Extras::forceActiveWindow(window);
    }
}

void XWindowTasksModel::requestNewVirtualDesktop(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());
    const int desktop = KX11Extras::numberOfDesktops() + 1;

    // FIXME Arbitrary limit of 20 copied from old code.
    if (desktop > 20) {
        return;
    }

    NETRootInfo ri(X11Info::connection(), NET::NumberOfDesktops);
    ri.setNumberOfDesktops(desktop);

    KX11Extras::setOnDesktop(window, desktop);
}

void XWindowTasksModel::requestActivities(const QModelIndex &index, const QStringList &activities)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());

    KX11Extras::setOnActivities(window, activities);
}

void XWindowTasksModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    Q_UNUSED(delegate)

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const WId window = d->windows.at(index.row());

    if (d->delegateGeometries.contains(window) && d->delegateGeometries.value(window) == geometry) {
        return;
    }

    NETWinInfo ni(X11Info::connection(), window, X11Info::appRootWindow(), NET::Properties(), NET::Properties2());
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
    WId id;
    if (data.size() != sizeof(WId)) {
        return 0;
    } else {
        memcpy(&id, data.data(), sizeof(WId));
    }

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
