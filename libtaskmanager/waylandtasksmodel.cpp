/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "waylandtasksmodel.h"
#include "libtaskmanager_debug.h"
#include "tasktools.h"
#include "virtualdesktopinfo.h"

#include <KDirWatch>
#include <KSharedConfig>
#include <KWindowSystem>

#include <qwayland-plasma-window-management.h>

#include <QFuture>
#include <QGuiApplication>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QUrl>
#include <QUuid>
#include <QWaylandClientExtension>
#include <QWindow>
#include <QtConcurrent>
#include <qpa/qplatformwindow_p.h>

#include <fcntl.h>
#include <sys/poll.h>
#include <unistd.h>

namespace TaskManager
{

class PlasmaWindow : public QObject, public QtWayland::org_kde_plasma_window
{
    Q_OBJECT
public:
    PlasmaWindow(const QString &uuid, ::org_kde_plasma_window *id)
        : org_kde_plasma_window(id)
        , uuid(uuid)
    {
    }
    ~PlasmaWindow()
    {
        destroy();
    }
    using state = QtWayland::org_kde_plasma_window_management::state;
    const QString uuid;
    QString title;
    QString appId;
    QIcon icon;
    QFlags<state> windowState;
    QList<QString> virtualDesktops;
    QRect geometry;
    QString applicationMenuService;
    QString applicationMenuObjectPath;
    QList<QString> activities;
    quint32 pid;
    QString resourceName;
    QPointer<PlasmaWindow> parentWindow;
    bool wasUnmapped = false;

Q_SIGNALS:
    void unmapped();
    void titleChanged();
    void appIdChanged();
    void iconChanged();
    void activeChanged();
    void minimizedChanged();
    void maximizedChanged();
    void fullscreenChanged();
    void keepAboveChanged();
    void keepBelowChanged();
    void onAllDesktopsChanged();
    void demandsAttentionChanged();
    void closeableChanged();
    void minimizeableChanged();
    void maximizeableChanged();
    void fullscreenableChanged();
    void skiptaskbarChanged();
    void shadeableChanged();
    void shadedChanged();
    void movableChanged();
    void resizableChanged();
    void virtualDesktopChangeableChanged();
    void skipSwitcherChanged();
    void virtualDesktopEntered();
    void virtualDesktopLeft();
    void geometryChanged();
    void skipTaskbarChanged();
    void applicationMenuChanged();
    void activitiesChanged();
    void parentWindowChanged();
    void initialStateDone();

protected:
    void org_kde_plasma_window_unmapped() override
    {
        wasUnmapped = true;
        Q_EMIT unmapped();
    }
    void org_kde_plasma_window_title_changed(const QString &title) override
    {
        this->title = title;
        Q_EMIT titleChanged();
    }
    void org_kde_plasma_window_app_id_changed(const QString &app_id) override
    {
        appId = app_id;
        Q_EMIT appIdChanged();
    }
    void org_kde_plasma_window_icon_changed() override
    {
        int pipeFds[2];
        if (pipe2(pipeFds, O_CLOEXEC) != 0) {
            qCWarning(TASKMANAGER_DEBUG) << "failed creating pipe";
            return;
        }
        get_icon(pipeFds[1]);
        ::close(pipeFds[1]);
        auto readIcon = [uuid = uuid](int fd) {
            pollfd pollFd;
            pollFd.fd = fd;
            pollFd.events = POLLIN;
            QByteArray data;
            while (true) {
                int ready = poll(&pollFd, 1, 1000);
                if (ready < 0 && errno != EINTR) {
                    qCWarning(TASKMANAGER_DEBUG) << "polling for icon of window" << uuid << "failed";
                    return QIcon();
                } else if (ready == 0) {
                    qCWarning(TASKMANAGER_DEBUG) << "time out polling for icon of window" << uuid;
                    return QIcon();
                } else {
                    char buffer[4096];
                    int n = read(fd, buffer, sizeof(buffer));
                    if (n < 0) {
                        qCWarning(TASKMANAGER_DEBUG) << "error reading icon of window" << uuid;
                        return QIcon();
                    } else if (n > 0) {
                        data.append(buffer, n);
                    } else {
                        QIcon icon;
                        QDataStream ds(data);
                        ds >> icon;
                        return icon;
                    }
                }
            }
        };
        QFuture<QIcon> future = QtConcurrent::run(readIcon, pipeFds[0]);
        auto watcher = new QFutureWatcher<QIcon>();
        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<QIcon>::finished, this, [this, watcher] {
            icon = watcher->future().result();
            Q_EMIT iconChanged();
        });
        connect(watcher, &QFutureWatcher<QIcon>::finished, watcher, &QObject::deleteLater);
    }
    void org_kde_plasma_window_themed_icon_name_changed(const QString &name) override
    {
        icon = QIcon::fromTheme(name);
        Q_EMIT iconChanged();
    }
    void org_kde_plasma_window_state_changed(uint32_t flags) override
    {
        auto diff = windowState ^ flags;
        if (diff & state::state_active) {
            windowState.setFlag(state::state_active, flags & state::state_active);
            Q_EMIT activeChanged();
        }
        if (diff & state::state_minimized) {
            windowState.setFlag(state::state_minimized, flags & state::state_minimized);
            Q_EMIT minimizedChanged();
        }
        if (diff & state::state_maximized) {
            windowState.setFlag(state::state_maximized, flags & state::state_maximized);
            Q_EMIT maximizedChanged();
        }
        if (diff & state::state_fullscreen) {
            windowState.setFlag(state::state_fullscreen, flags & state::state_fullscreen);
            Q_EMIT fullscreenChanged();
        }
        if (diff & state::state_keep_above) {
            windowState.setFlag(state::state_keep_above, flags & state::state_keep_above);
            Q_EMIT keepAboveChanged();
        }
        if (diff & state::state_keep_below) {
            windowState.setFlag(state::state_keep_below, flags & state::state_keep_below);
            Q_EMIT keepBelowChanged();
        }
        if (diff & state::state_on_all_desktops) {
            windowState.setFlag(state::state_on_all_desktops, flags & state::state_on_all_desktops);
            Q_EMIT onAllDesktopsChanged();
        }
        if (diff & state::state_demands_attention) {
            windowState.setFlag(state::state_demands_attention, flags & state::state_demands_attention);
            Q_EMIT demandsAttentionChanged();
        }
        if (diff & state::state_closeable) {
            windowState.setFlag(state::state_closeable, flags & state::state_closeable);
            Q_EMIT closeableChanged();
        }
        if (diff & state::state_minimizable) {
            windowState.setFlag(state::state_minimizable, flags & state::state_minimizable);
            Q_EMIT minimizeableChanged();
        }
        if (diff & state::state_maximizable) {
            windowState.setFlag(state::state_maximizable, flags & state::state_maximizable);
            Q_EMIT maximizeableChanged();
        }
        if (diff & state::state_fullscreenable) {
            windowState.setFlag(state::state_fullscreenable, flags & state::state_fullscreenable);
            Q_EMIT fullscreenableChanged();
        }
        if (diff & state::state_skiptaskbar) {
            windowState.setFlag(state::state_skiptaskbar, flags & state::state_skiptaskbar);
            Q_EMIT skipTaskbarChanged();
        }
        if (diff & state::state_shadeable) {
            windowState.setFlag(state::state_shadeable, flags & state::state_shadeable);
            Q_EMIT shadeableChanged();
        }
        if (diff & state::state_shaded) {
            windowState.setFlag(state::state_shaded, flags & state::state_shaded);
            Q_EMIT shadedChanged();
        }
        if (diff & state::state_movable) {
            windowState.setFlag(state::state_movable, flags & state::state_movable);
            Q_EMIT movableChanged();
        }
        if (diff & state::state_resizable) {
            windowState.setFlag(state::state_resizable, flags & state::state_resizable);
            Q_EMIT resizableChanged();
        }
        if (diff & state::state_virtual_desktop_changeable) {
            windowState.setFlag(state::state_virtual_desktop_changeable, flags & state::state_virtual_desktop_changeable);
            Q_EMIT virtualDesktopChangeableChanged();
        }
        if (diff & state::state_skipswitcher) {
            windowState.setFlag(state::state_skipswitcher, flags & state::state_skipswitcher);
            Q_EMIT skipSwitcherChanged();
        }
    }
    void org_kde_plasma_window_virtual_desktop_entered(const QString &id) override
    {
        virtualDesktops.push_back(id);
        Q_EMIT virtualDesktopEntered();
    }

    void org_kde_plasma_window_virtual_desktop_left(const QString &id) override
    {
        virtualDesktops.removeAll(id);
        Q_EMIT virtualDesktopLeft();
    }
    void org_kde_plasma_window_geometry(int32_t x, int32_t y, uint32_t width, uint32_t height) override
    {
        geometry = QRect(x, y, width, height);
        Q_EMIT geometryChanged();
    }
    void org_kde_plasma_window_application_menu(const QString &service_name, const QString &object_path) override

    {
        applicationMenuService = service_name;
        applicationMenuObjectPath = object_path;
        Q_EMIT applicationMenuChanged();
    }
    void org_kde_plasma_window_activity_entered(const QString &id) override
    {
        activities.push_back(id);
        Q_EMIT activitiesChanged();
    }
    void org_kde_plasma_window_activity_left(const QString &id) override
    {
        activities.removeAll(id);
        Q_EMIT activitiesChanged();
    }
    void org_kde_plasma_window_pid_changed(uint32_t pid) override
    {
        this->pid = pid;
    }
    void org_kde_plasma_window_resource_name_changed(const QString &resource_name) override
    {
        resourceName = resource_name;
    }
    void org_kde_plasma_window_parent_window(::org_kde_plasma_window *parent) override
    {
        PlasmaWindow *parentWindow = nullptr;
        if (parent) {
            parentWindow = dynamic_cast<PlasmaWindow *>(PlasmaWindow::fromObject(parent));
        }
        setParentWindow(parentWindow);
    }
    void org_kde_plasma_window_initial_state() override
    {
        Q_EMIT initialStateDone();
    }

private:
    void setParentWindow(PlasmaWindow *parent)
    {
        const auto old = parentWindow;
        QObject::disconnect(parentWindowUnmappedConnection);

        if (parent && !parent->wasUnmapped) {
            parentWindow = QPointer<PlasmaWindow>(parent);
            parentWindowUnmappedConnection = QObject::connect(parent, &PlasmaWindow::unmapped, this, [this] {
                setParentWindow(nullptr);
            });
        } else {
            parentWindow = QPointer<PlasmaWindow>();
            parentWindowUnmappedConnection = QMetaObject::Connection();
        }

        if (parentWindow.data() != old.data()) {
            Q_EMIT parentWindowChanged();
        }
    }

    QMetaObject::Connection parentWindowUnmappedConnection;
};

class PlasmaWindowManagement : public QWaylandClientExtensionTemplate<PlasmaWindowManagement>, public QtWayland::org_kde_plasma_window_management
{
    Q_OBJECT
public:
    static constexpr int version = 16;
    PlasmaWindowManagement()
        : QWaylandClientExtensionTemplate(version)
    {
        connect(this, &QWaylandClientExtension::activeChanged, this, [this] {
            if (!isActive()) {
                wl_proxy_destroy(reinterpret_cast<wl_proxy *>(object()));
            }
        });
    }
    ~PlasmaWindowManagement()
    {
        if (isActive()) {
            wl_proxy_destroy(reinterpret_cast<wl_proxy *>(object()));
        }
    }
    void org_kde_plasma_window_management_window_with_uuid(uint32_t id, const QString &uuid) override
    {
        Q_UNUSED(id)
        Q_EMIT windowCreated(new PlasmaWindow(uuid, get_window_by_uuid(uuid)));
    }
    void org_kde_plasma_window_management_stacking_order_uuid_changed(const QString &uuids) override
    {
        Q_EMIT stackingOrderChanged(uuids);
    }
Q_SIGNALS:
    void windowCreated(PlasmaWindow *window);
    void stackingOrderChanged(const QString &uuids);
};
class Q_DECL_HIDDEN WaylandTasksModel::Private
{
public:
    Private(WaylandTasksModel *q);
    QHash<PlasmaWindow *, AppData> appDataCache;
    QHash<PlasmaWindow *, QTime> lastActivated;
    PlasmaWindow *activeWindow = nullptr;
    std::vector<std::unique_ptr<PlasmaWindow>> windows;
    // key=transient child, value=leader
    QHash<PlasmaWindow *, PlasmaWindow *> transients;
    // key=leader, values=transient children
    QMultiHash<PlasmaWindow *, PlasmaWindow *> transientsDemandingAttention;
    std::unique_ptr<PlasmaWindowManagement> windowManagement;
    KSharedConfig::Ptr rulesConfig;
    KDirWatch *configWatcher = nullptr;
    VirtualDesktopInfo *virtualDesktopInfo = nullptr;
    static QUuid uuid;
    QList<QString> stackingOrder;

    void init();
    void initWayland();
    auto findWindow(PlasmaWindow *window) const;
    void addWindow(PlasmaWindow *window);

    AppData appData(PlasmaWindow *window);

    QIcon icon(PlasmaWindow *window);

    static QString mimeType();
    static QString groupMimeType();

    void dataChanged(PlasmaWindow *window, int role);
    void dataChanged(PlasmaWindow *window, const QList<int> &roles);

private:
    WaylandTasksModel *q;
};

QUuid WaylandTasksModel::Private::uuid = QUuid::createUuid();

WaylandTasksModel::Private::Private(WaylandTasksModel *q)
    : q(q)
{
}

void WaylandTasksModel::Private::init()
{
    auto clearCacheAndRefresh = [this] {
        if (windows.empty()) {
            return;
        }

        appDataCache.clear();

        // Emit changes of all roles satisfied from app data cache.
        Q_EMIT q->dataChanged(q->index(0, 0),
                              q->index(windows.size() - 1, 0),
                              QList<int>{Qt::DecorationRole,
                                         AbstractTasksModel::AppId,
                                         AbstractTasksModel::AppName,
                                         AbstractTasksModel::GenericName,
                                         AbstractTasksModel::LauncherUrl,
                                         AbstractTasksModel::LauncherUrlWithoutIcon,
                                         AbstractTasksModel::CanLaunchNewInstance,
                                         AbstractTasksModel::SkipTaskbar});
    };

    rulesConfig = KSharedConfig::openConfig(QStringLiteral("taskmanagerrulesrc"));
    configWatcher = new KDirWatch(q);

    for (const QString &location : QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)) {
        configWatcher->addFile(location + QLatin1String("/taskmanagerrulesrc"));
    }

    auto rulesConfigChange = [this, clearCacheAndRefresh] {
        rulesConfig->reparseConfiguration();
        clearCacheAndRefresh();
    };

    QObject::connect(configWatcher, &KDirWatch::dirty, rulesConfigChange);
    QObject::connect(configWatcher, &KDirWatch::created, rulesConfigChange);
    QObject::connect(configWatcher, &KDirWatch::deleted, rulesConfigChange);

    virtualDesktopInfo = new VirtualDesktopInfo(q);

    initWayland();
}

void WaylandTasksModel::Private::initWayland()
{
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }

    windowManagement = std::make_unique<PlasmaWindowManagement>();

    QObject::connect(windowManagement.get(), &PlasmaWindowManagement::activeChanged, q, [this] {
        q->beginResetModel();
        windows.clear();
        q->endResetModel();
    });

    QObject::connect(windowManagement.get(), &PlasmaWindowManagement::windowCreated, q, [this](PlasmaWindow *window) {
        connect(window, &PlasmaWindow::initialStateDone, q, [this, window] {
            addWindow(window);
        });
    });

    QObject::connect(windowManagement.get(), &PlasmaWindowManagement::stackingOrderChanged, q, [this](const QString &order) {
        stackingOrder = order.split(QLatin1Char(';'));
        for (const auto &window : std::as_const(windows)) {
            this->dataChanged(window.get(), StackingOrder);
        }
    });
}

auto WaylandTasksModel::Private::findWindow(PlasmaWindow *window) const
{
    return std::find_if(windows.begin(), windows.end(), [window](const std::unique_ptr<PlasmaWindow> &candidate) {
        return candidate.get() == window;
    });
}

void WaylandTasksModel::Private::addWindow(PlasmaWindow *window)
{
    if (findWindow(window) != windows.end() || transients.contains(window)) {
        return;
    }

    // Handle transient.
    if (PlasmaWindow *leader = window->parentWindow.data()) {
        transients.insert(window, leader);

        // Update demands attention state for leader.
        if (window->windowState.testFlag(PlasmaWindow::state::state_demands_attention)) {
            transientsDemandingAttention.insert(leader, window);
            dataChanged(leader, QVector<int>{IsDemandingAttention});
        }
    } else {
        const int count = windows.size();

        q->beginInsertRows(QModelIndex(), count, count);

        windows.emplace_back(window);

        q->endInsertRows();
    }

    auto removeWindow = [window, this] {
        auto it = findWindow(window);
        if (it != windows.end()) {
            const int row = it - windows.begin();
            q->beginRemoveRows(QModelIndex(), row, row);
            windows.erase(it);
            transientsDemandingAttention.remove(window);
            appDataCache.remove(window);
            lastActivated.remove(window);
            q->endRemoveRows();
        } else { // Could be a transient.
            // Removing a transient might change the demands attention state of the leader.
            if (transients.remove(window)) {
                if (PlasmaWindow *leader = transientsDemandingAttention.key(window)) {
                    transientsDemandingAttention.remove(leader, window);
                    dataChanged(leader, QVector<int>{IsDemandingAttention});
                }
            }
        }

        if (activeWindow == window) {
            activeWindow = nullptr;
        }
    };

    QObject::connect(window, &PlasmaWindow::unmapped, q, removeWindow);

    QObject::connect(window, &PlasmaWindow::titleChanged, q, [window, this] {
        this->dataChanged(window, Qt::DisplayRole);
    });

    QObject::connect(window, &PlasmaWindow::iconChanged, q, [window, this] {
        // The icon in the AppData struct might come from PlasmaWindow if it wasn't
        // filled in by windowUrlFromMetadata+appDataFromUrl.
        // TODO: Don't evict the cache unnecessarily if this isn't the case. As icons
        // are currently very static on Wayland, this eviction is unlikely to happen
        // frequently as of now.
        appDataCache.remove(window);
        this->dataChanged(window, Qt::DecorationRole);
    });

    QObject::connect(window, &PlasmaWindow::appIdChanged, q, [window, this] {
        // The AppData struct in the cache is derived from this and needs
        // to be evicted in favor of a fresh struct based on the changed
        // window metadata.
        appDataCache.remove(window);

        // Refresh roles satisfied from the app data cache.
        this->dataChanged(window,
                          QList<int>{Qt::DecorationRole, AppId, AppName, GenericName, LauncherUrl, LauncherUrlWithoutIcon, SkipTaskbar, CanLaunchNewInstance});
    });

    QObject::connect(window, &PlasmaWindow::activeChanged, q, [window, this] {
        const bool active = window->windowState & PlasmaWindow::state::state_active;

        PlasmaWindow *effectiveWindow = window;

        while (effectiveWindow->parentWindow) {
            effectiveWindow = effectiveWindow->parentWindow;
        }

        if (active) {
            lastActivated[effectiveWindow] = QTime::currentTime();

            if (activeWindow != effectiveWindow) {
                activeWindow = effectiveWindow;
                this->dataChanged(effectiveWindow, IsActive);
            }
        } else {
            if (activeWindow == effectiveWindow) {
                activeWindow = nullptr;
                this->dataChanged(effectiveWindow, IsActive);
            }
        }
    });

    QObject::connect(window, &PlasmaWindow::parentWindowChanged, q, [window, this] {
        PlasmaWindow *leader = window->parentWindow.data();

        // Migrate demanding attention to new leader.
        if (window->windowState.testFlag(PlasmaWindow::state::state_demands_attention)) {
            if (auto *oldLeader = transientsDemandingAttention.key(window)) {
                if (window->parentWindow != oldLeader) {
                    transientsDemandingAttention.remove(oldLeader, window);
                    transientsDemandingAttention.insert(leader, window);
                    dataChanged(oldLeader, QVector<int>{IsDemandingAttention});
                    dataChanged(leader, QVector<int>{IsDemandingAttention});
                }
            }
        }

        // TODO handle parent change,
        // migrate to new leader,
        // or add/remove proper window if it got/lost a leader.
    });

    QObject::connect(window, &PlasmaWindow::closeableChanged, q, [window, this] {
        this->dataChanged(window, IsClosable);
    });

    QObject::connect(window, &PlasmaWindow::movableChanged, q, [window, this] {
        this->dataChanged(window, IsMovable);
    });

    QObject::connect(window, &PlasmaWindow::resizableChanged, q, [window, this] {
        this->dataChanged(window, IsResizable);
    });

    QObject::connect(window, &PlasmaWindow::fullscreenableChanged, q, [window, this] {
        this->dataChanged(window, IsFullScreenable);
    });

    QObject::connect(window, &PlasmaWindow::fullscreenChanged, q, [window, this] {
        this->dataChanged(window, IsFullScreen);
    });

    QObject::connect(window, &PlasmaWindow::maximizeableChanged, q, [window, this] {
        this->dataChanged(window, IsMaximizable);
    });

    QObject::connect(window, &PlasmaWindow::maximizedChanged, q, [window, this] {
        this->dataChanged(window, IsMaximized);
    });

    QObject::connect(window, &PlasmaWindow::minimizeableChanged, q, [window, this] {
        this->dataChanged(window, IsMinimizable);
    });

    QObject::connect(window, &PlasmaWindow::minimizedChanged, q, [window, this] {
        this->dataChanged(window, IsMinimized);
    });

    QObject::connect(window, &PlasmaWindow::keepAboveChanged, q, [window, this] {
        this->dataChanged(window, IsKeepAbove);
    });

    QObject::connect(window, &PlasmaWindow::keepBelowChanged, q, [window, this] {
        this->dataChanged(window, IsKeepBelow);
    });

    QObject::connect(window, &PlasmaWindow::shadeableChanged, q, [window, this] {
        this->dataChanged(window, IsShadeable);
    });

    QObject::connect(window, &PlasmaWindow::virtualDesktopChangeableChanged, q, [window, this] {
        this->dataChanged(window, IsVirtualDesktopsChangeable);
    });

    QObject::connect(window, &PlasmaWindow::virtualDesktopEntered, q, [window, this] {
        this->dataChanged(window, VirtualDesktops);

        // If the count has changed from 0, the window may no longer be on all virtual
        // desktops.
        if (window->virtualDesktops.count() > 0) {
            this->dataChanged(window, IsOnAllVirtualDesktops);
        }
    });

    QObject::connect(window, &PlasmaWindow::virtualDesktopLeft, q, [window, this] {
        this->dataChanged(window, VirtualDesktops);

        // If the count has changed to 0, the window is now on all virtual desktops.
        if (window->virtualDesktops.count() == 0) {
            this->dataChanged(window, IsOnAllVirtualDesktops);
        }
    });

    QObject::connect(window, &PlasmaWindow::geometryChanged, q, [window, this] {
        this->dataChanged(window, QList<int>{Geometry, ScreenGeometry});
    });

    QObject::connect(window, &PlasmaWindow::demandsAttentionChanged, q, [window, this] {
        // Changes to a transient's state might change demands attention state for leader.
        if (auto *leader = transients.value(window)) {
            if (window->windowState.testFlag(PlasmaWindow::state::state_demands_attention)) {
                if (!transientsDemandingAttention.values(leader).contains(window)) {
                    transientsDemandingAttention.insert(leader, window);
                    this->dataChanged(leader, QVector<int>{IsDemandingAttention});
                }
            } else if (transientsDemandingAttention.remove(window)) {
                this->dataChanged(leader, QVector<int>{IsDemandingAttention});
            }
        } else {
            this->dataChanged(window, QVector<int>{IsDemandingAttention});
        }
    });

    QObject::connect(window, &PlasmaWindow::skipTaskbarChanged, q, [window, this] {
        this->dataChanged(window, SkipTaskbar);
    });

    QObject::connect(window, &PlasmaWindow::applicationMenuChanged, q, [window, this] {
        this->dataChanged(window, QList<int>{ApplicationMenuServiceName, ApplicationMenuObjectPath});
    });

    QObject::connect(window, &PlasmaWindow::activitiesChanged, q, [window, this] {
        this->dataChanged(window, Activities);
    });
}

AppData WaylandTasksModel::Private::appData(PlasmaWindow *window)
{
    const auto &it = appDataCache.constFind(window);

    if (it != appDataCache.constEnd()) {
        return *it;
    }

    const AppData &data = appDataFromUrl(windowUrlFromMetadata(window->appId, window->pid, rulesConfig, window->resourceName));

    appDataCache.insert(window, data);

    return data;
}

QIcon WaylandTasksModel::Private::icon(PlasmaWindow *window)
{
    const AppData &app = appData(window);

    if (!app.icon.isNull()) {
        return app.icon;
    }

    appDataCache[window].icon = window->icon;

    return window->icon;
}

QString WaylandTasksModel::Private::mimeType()
{
    // Use a unique format id to make this intentionally useless for
    // cross-process DND.
    return QStringLiteral("windowsystem/winid+") + uuid.toString();
}

QString WaylandTasksModel::Private::groupMimeType()
{
    // Use a unique format id to make this intentionally useless for
    // cross-process DND.
    return QStringLiteral("windowsystem/multiple-winids+") + uuid.toString();
}

void WaylandTasksModel::Private::dataChanged(PlasmaWindow *window, int role)
{
    auto it = findWindow(window);
    if (it == windows.end()) {
        return;
    }
    QModelIndex idx = q->index(it - windows.begin());
    Q_EMIT q->dataChanged(idx, idx, QList<int>{role});
}

void WaylandTasksModel::Private::dataChanged(PlasmaWindow *window, const QList<int> &roles)
{
    auto it = findWindow(window);
    if (it == windows.end()) {
        return;
    }
    QModelIndex idx = q->index(it - windows.begin());
    Q_EMIT q->dataChanged(idx, idx, roles);
}

WaylandTasksModel::WaylandTasksModel(QObject *parent)
    : AbstractWindowTasksModel(parent)
    , d(new Private(this))
{
    d->init();
}

WaylandTasksModel::~WaylandTasksModel() = default;

QVariant WaylandTasksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= d->windows.size()) {
        return QVariant();
    }

    PlasmaWindow *window = d->windows.at(index.row()).get();

    if (role == Qt::DisplayRole) {
        return window->title;
    } else if (role == Qt::DecorationRole) {
        return d->icon(window);
    } else if (role == AppId) {
        const QString &id = d->appData(window).id;

        if (id.isEmpty()) {
            return window->appId;
        } else {
            return id;
        }
    } else if (role == AppName) {
        return d->appData(window).name;
    } else if (role == GenericName) {
        return d->appData(window).genericName;
    } else if (role == LauncherUrl || role == LauncherUrlWithoutIcon) {
        return d->appData(window).url;
    } else if (role == WinIdList) {
        return QVariantList{window->uuid};
    } else if (role == MimeType) {
        return d->mimeType();
    } else if (role == MimeData) {
        return window->uuid;
    } else if (role == IsWindow) {
        return true;
    } else if (role == IsActive) {
        return (window == d->activeWindow);
    } else if (role == IsClosable) {
        return window->windowState.testFlag(PlasmaWindow::state::state_closeable);
    } else if (role == IsMovable) {
        return window->windowState.testFlag(PlasmaWindow::state::state_movable);
    } else if (role == IsResizable) {
        return window->windowState.testFlag(PlasmaWindow::state::state_resizable);
    } else if (role == IsMaximizable) {
        return window->windowState.testFlag(PlasmaWindow::state::state_maximizable);
    } else if (role == IsMaximized) {
        return window->windowState.testFlag(PlasmaWindow::state::state_maximized);
    } else if (role == IsMinimizable) {
        return window->windowState.testFlag(PlasmaWindow::state::state_minimizable);
    } else if (role == IsMinimized || role == IsHidden) {
        return window->windowState.testFlag(PlasmaWindow::state::state_minimized);
    } else if (role == IsKeepAbove) {
        return window->windowState.testFlag(PlasmaWindow::state::state_keep_above);
    } else if (role == IsKeepBelow) {
        return window->windowState.testFlag(PlasmaWindow::state::state_keep_below);
    } else if (role == IsFullScreenable) {
        return window->windowState.testFlag(PlasmaWindow::state::state_fullscreenable);
    } else if (role == IsFullScreen) {
        return window->windowState.testFlag(PlasmaWindow::state::state_fullscreen);
    } else if (role == IsShadeable) {
        return window->windowState.testFlag(PlasmaWindow::state::state_shadeable);
    } else if (role == IsShaded) {
        return window->windowState.testFlag(PlasmaWindow::state::state_shaded);
    } else if (role == IsVirtualDesktopsChangeable) {
        return window->windowState.testFlag(PlasmaWindow::state::state_virtual_desktop_changeable);
    } else if (role == VirtualDesktops) {
        return window->virtualDesktops;
    } else if (role == IsOnAllVirtualDesktops) {
        return window->virtualDesktops.isEmpty();
    } else if (role == Geometry) {
        return window->geometry;
    } else if (role == ScreenGeometry) {
        return screenGeometry(window->geometry.center());
    } else if (role == Activities) {
        return window->activities;
    } else if (role == IsDemandingAttention) {
        return window->windowState.testFlag(PlasmaWindow::state::state_demands_attention) || d->transientsDemandingAttention.contains(window);
    } else if (role == SkipTaskbar) {
        return window->windowState.testFlag(PlasmaWindow::state::state_skiptaskbar) || d->appData(window).skipTaskbar;
    } else if (role == SkipPager) {
        // FIXME Implement.
    } else if (role == AppPid) {
        return window->pid;
    } else if (role == StackingOrder) {
        return d->stackingOrder.indexOf(window->uuid);
    } else if (role == LastActivated) {
        if (d->lastActivated.contains(window)) {
            return d->lastActivated.value(window);
        }
    } else if (role == ApplicationMenuObjectPath) {
        return window->applicationMenuObjectPath;
    } else if (role == ApplicationMenuServiceName) {
        return window->applicationMenuService;
    } else if (role == CanLaunchNewInstance) {
        return canLauchNewInstance(d->appData(window));
    }

    return AbstractTasksModel::data(index, role);
}

int WaylandTasksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->windows.size();
}

QModelIndex WaylandTasksModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column, d->windows.at(row).get()) : QModelIndex();
}

void WaylandTasksModel::requestActivate(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    PlasmaWindow *window = d->windows.at(index.row()).get();

    // Pull forward any transient demanding attention.
    if (auto *transientDemandingAttention = d->transientsDemandingAttention.value(window)) {
        window = transientDemandingAttention;
    } else {
        // TODO Shouldn't KWin take care of that?
        // Bringing a transient to the front usually brings its parent with it
        // but focus is not handled properly.
        // TODO take into account d->lastActivation instead
        // of just taking the first one.
        while (d->transients.key(window)) {
            window = d->transients.key(window);
        }
    }

    window->set_state(PlasmaWindow::state::state_active, PlasmaWindow::state::state_active);
}

void WaylandTasksModel::requestNewInstance(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    runApp(d->appData(d->windows.at(index.row()).get()));
}

void WaylandTasksModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent) || urls.isEmpty()) {
        return;
    }

    runApp(d->appData(d->windows.at(index.row()).get()), urls);
}

void WaylandTasksModel::requestClose(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    d->windows.at(index.row())->close();
}

void WaylandTasksModel::requestMove(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    auto &window = d->windows.at(index.row());

    window->set_state(PlasmaWindow::state::state_active, PlasmaWindow::state::state_active);
    window->request_move();
}

void WaylandTasksModel::requestResize(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    auto &window = d->windows.at(index.row());

    window->set_state(PlasmaWindow::state::state_active, PlasmaWindow::state::state_active);
    window->request_resize();
}

void WaylandTasksModel::requestToggleMinimized(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    auto &window = d->windows.at(index.row());

    if (window->windowState & PlasmaWindow::state::state_minimized) {
        window->set_state(PlasmaWindow::state::state_minimized, 0);
    } else {
        window->set_state(PlasmaWindow::state::state_minimized, PlasmaWindow::state::state_minimized);
    }
}

void WaylandTasksModel::requestToggleMaximized(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    auto &window = d->windows.at(index.row());

    if (window->windowState & PlasmaWindow::state::state_maximized) {
        window->set_state(PlasmaWindow::state::state_maximized | PlasmaWindow::state::state_active, PlasmaWindow::state::state_active);
    } else {
        window->set_state(PlasmaWindow::state::state_maximized | PlasmaWindow::state::state_active,
                          PlasmaWindow::state::state_maximized | PlasmaWindow::state::state_active);
    }
}

void WaylandTasksModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    auto &window = d->windows.at(index.row());

    if (window->windowState & PlasmaWindow::state::state_keep_above) {
        window->set_state(PlasmaWindow::state::state_keep_above, 0);
    } else {
        window->set_state(PlasmaWindow::state::state_keep_above, PlasmaWindow::state::state_keep_above);
    }
}

void WaylandTasksModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }
    auto &window = d->windows.at(index.row());

    if (window->windowState & PlasmaWindow::state::state_keep_below) {
        window->set_state(PlasmaWindow::state::state_keep_below, 0);
    } else {
        window->set_state(PlasmaWindow::state::state_keep_below, PlasmaWindow::state::state_keep_below);
    }
}

void WaylandTasksModel::requestToggleFullScreen(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    auto &window = d->windows.at(index.row());

    if (window->windowState & PlasmaWindow::state::state_fullscreen) {
        window->set_state(PlasmaWindow::state::state_fullscreen, 0);
    } else {
        window->set_state(PlasmaWindow::state::state_fullscreen, PlasmaWindow::state::state_fullscreen);
    }
}

void WaylandTasksModel::requestToggleShaded(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    auto &window = d->windows.at(index.row());

    if (window->windowState & PlasmaWindow::state::state_shaded) {
        window->set_state(PlasmaWindow::state::state_shaded, 0);
    } else {
        window->set_state(PlasmaWindow::state::state_shaded, PlasmaWindow::state::state_shaded);
    };
}

void WaylandTasksModel::requestVirtualDesktops(const QModelIndex &index, const QVariantList &desktops)
{
    // FIXME TODO: Lacks the "if we've requested the current desktop, force-activate
    // the window" logic from X11 version. This behavior should be in KWin rather than
    // libtm however.

    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    auto &window = d->windows.at(index.row());

    if (desktops.isEmpty()) {
        const QStringList virtualDesktops = window->virtualDesktops;
        for (const QString &desktop : virtualDesktops) {
            window->request_leave_virtual_desktop(desktop);
        }
    } else {
        const QStringList &now = window->virtualDesktops;
        QStringList next;

        for (const QVariant &desktop : desktops) {
            const QString &desktopId = desktop.toString();

            if (!desktopId.isEmpty()) {
                next << desktopId;

                if (!now.contains(desktopId)) {
                    window->request_enter_virtual_desktop(desktopId);
                }
            }
        }

        for (const QString &desktop : now) {
            if (!next.contains(desktop)) {
                window->request_leave_virtual_desktop(desktop);
            }
        }
    }
}

void WaylandTasksModel::requestNewVirtualDesktop(const QModelIndex &index)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    d->windows.at(index.row())->request_enter_new_virtual_desktop();
}

void WaylandTasksModel::requestActivities(const QModelIndex &index, const QStringList &activities)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    auto &window = d->windows.at(index.row());
    const auto newActivities = QSet(activities.begin(), activities.end());
    const auto plasmaActivities = window->activities;
    const auto oldActivities = QSet(plasmaActivities.begin(), plasmaActivities.end());

    const auto activitiesToAdd = newActivities - oldActivities;
    for (const auto &activity : activitiesToAdd) {
        window->request_enter_activity(activity);
    }

    const auto activitiesToRemove = oldActivities - newActivities;
    for (const auto &activity : activitiesToRemove) {
        window->request_leave_activity(activity);
    }
}

void WaylandTasksModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    /*
    FIXME: This introduces the dependency on Qt::Quick. I might prefer
    reversing this and publishing the window pointer through the model,
    then calling PlasmaWindow::setMinimizeGeometry in the applet backend,
    rather than hand delegate items into the lib, keeping the lib more UI-
    agnostic.
    */

    Q_UNUSED(geometry)

    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::DoNotUseParent)) {
        return;
    }

    const QQuickItem *item = qobject_cast<const QQuickItem *>(delegate);

    if (!item || !item->parentItem()) {
        return;
    }

    QWindow *itemWindow = item->window();

    if (!itemWindow) {
        return;
    }

    auto waylandWindow = itemWindow->nativeInterface<QNativeInterface::Private::QWaylandWindow>();

    if (!waylandWindow || !waylandWindow->surface()) {
        return;
    }

    QRect rect(item->x(), item->y(), item->width(), item->height());
    rect.moveTopLeft(item->parentItem()->mapToScene(rect.topLeft()).toPoint());

    auto &window = d->windows.at(index.row());

    window->set_minimized_geometry(waylandWindow->surface(), rect.x(), rect.y(), rect.width(), rect.height());
}

QUuid WaylandTasksModel::winIdFromMimeData(const QMimeData *mimeData, bool *ok)
{
    Q_ASSERT(mimeData);

    if (ok) {
        *ok = false;
    }

    if (!mimeData->hasFormat(Private::mimeType())) {
        return {};
    }

    QUuid id(mimeData->data(Private::mimeType()));
    *ok = !id.isNull();

    return id;
}

QList<QUuid> WaylandTasksModel::winIdsFromMimeData(const QMimeData *mimeData, bool *ok)
{
    Q_ASSERT(mimeData);
    QList<QUuid> ids;

    if (ok) {
        *ok = false;
    }

    if (!mimeData->hasFormat(Private::groupMimeType())) {
        // Try to extract single window id.
        bool singularOk;
        QUuid id = winIdFromMimeData(mimeData, &singularOk);

        if (ok) {
            *ok = singularOk;
        }

        if (singularOk) {
            ids << id;
        }

        return ids;
    }

    // FIXME: Extracting multiple winids is still unimplemented;
    // TaskGroupingProxy::data(..., ::MimeData) can't produce
    // a payload with them anyways.

    return ids;
}

}

#include "waylandtasksmodel.moc"
