/********************************************************************
Copyright 2016  Eike Hein <hein@kde.org>

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

#include "waylandtasksmodel.h"
#include "tasktools.h"
#include "virtualdesktopinfo.h"

#include <KDirWatch>
#include <KService>
#include <KSharedConfig>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>

#include <QGuiApplication>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QUrl>
#include <QUuid>
#include <QWindow>

namespace TaskManager
{

class Q_DECL_HIDDEN WaylandTasksModel::Private
{
public:
    Private(WaylandTasksModel *q);
    QList<KWayland::Client::PlasmaWindow*> windows;
    QHash<KWayland::Client::PlasmaWindow*, AppData> appDataCache;
    KWayland::Client::PlasmaWindowManagement *windowManagement = nullptr;
    KSharedConfig::Ptr rulesConfig;
    KDirWatch *configWatcher = nullptr;
    VirtualDesktopInfo *virtualDesktopInfo = nullptr;
    static QUuid uuid;

    void init();
    void initWayland();
    void addWindow(KWayland::Client::PlasmaWindow *window);

    AppData appData(KWayland::Client::PlasmaWindow *window);

    QIcon icon(KWayland::Client::PlasmaWindow *window);

    static QString mimeType();
    static QString groupMimeType();

    void dataChanged(KWayland::Client::PlasmaWindow *window, int role);
    void dataChanged(KWayland::Client::PlasmaWindow *window, const QVector<int> &roles);

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
        if (!windows.count()) {
            return;
        }

        appDataCache.clear();

        // Emit changes of all roles satisfied from app data cache.
        q->dataChanged(q->index(0, 0),  q->index(windows.count() - 1, 0),
            QVector<int>{Qt::DecorationRole, AbstractTasksModel::AppId,
            AbstractTasksModel::AppName, AbstractTasksModel::GenericName,
            AbstractTasksModel::LauncherUrl,
            AbstractTasksModel::LauncherUrlWithoutIcon,
            AbstractTasksModel::SkipTaskbar});
    };

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

    virtualDesktopInfo = new VirtualDesktopInfo(q);

    initWayland();
}

void WaylandTasksModel::Private::initWayland()
{
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }

    KWayland::Client::ConnectionThread *connection = KWayland::Client::ConnectionThread::fromApplication(q);

    if (!connection) {
        return;
    }

    KWayland::Client::Registry *registry = new KWayland::Client::Registry(q);
    registry->create(connection);

    QObject::connect(registry, &KWayland::Client::Registry::plasmaWindowManagementAnnounced, q, [this, registry] (quint32 name, quint32 version) {
            windowManagement = registry->createPlasmaWindowManagement(name, version, q);

            QObject::connect(windowManagement, &KWayland::Client::PlasmaWindowManagement::interfaceAboutToBeReleased, q,
                [this] {
                    q->beginResetModel();
                    windows.clear();
                    q->endResetModel();
                }
            );

            QObject::connect(windowManagement, &KWayland::Client::PlasmaWindowManagement::windowCreated, q,
                [this](KWayland::Client::PlasmaWindow *window) {
                    addWindow(window);
                }
            );

            const auto windows = windowManagement->windows();
            for (auto it = windows.constBegin(); it != windows.constEnd(); ++it) {
                addWindow(*it);
            }
        }
    );

    registry->setup();
}

void WaylandTasksModel::Private::addWindow(KWayland::Client::PlasmaWindow *window)
{
    if (windows.indexOf(window) != -1) {
        return;
    }

    const int count = windows.count();

    q->beginInsertRows(QModelIndex(), count, count);

    windows.append(window);

    q->endInsertRows();

    auto removeWindow = [window, this] {
        const int row = windows.indexOf(window);
        if (row != -1) {
            q->beginRemoveRows(QModelIndex(), row, row);
            windows.removeAt(row);
            appDataCache.remove(window);
            q->endRemoveRows();
        }
    };

    QObject::connect(window, &KWayland::Client::PlasmaWindow::unmapped, q, removeWindow);
    QObject::connect(window, &QObject::destroyed, q, removeWindow);

    QObject::connect(window, &KWayland::Client::PlasmaWindow::titleChanged, q,
        [window, this] { this->dataChanged(window, Qt::DisplayRole); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::iconChanged, q,
        [window, this] {
            // The icon in the AppData struct might come from PlasmaWindow if it wasn't
            // filled in by windowUrlFromMetadata+appDataFromUrl.
            // TODO: Don't evict the cache unnecessarily if this isn't the case. As icons
            // are currently very static on Wayland, this eviction is unlikely to happen
            // frequently as of now.
            appDataCache.remove(window);

            this->dataChanged(window, Qt::DecorationRole);
        }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::appIdChanged, q,
        [window, this] {
            // The AppData struct in the cache is derived from this and needs
            // to be evicted in favor of a fresh struct based on the changed
            // window metadata.
            appDataCache.remove(window);

            // Refresh roles satisfied from the app data cache.
            this->dataChanged(window, QVector<int>{AppId, AppName, GenericName,
                LauncherUrl, LauncherUrlWithoutIcon, SkipTaskbar});
        }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::activeChanged, q,
        [window, this] { this->dataChanged(window, IsActive); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::closeableChanged, q,
        [window, this] { this->dataChanged(window, IsClosable); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::movableChanged, q,
        [window, this] { this->dataChanged(window, IsMovable); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::resizableChanged, q,
        [window, this] { this->dataChanged(window, IsResizable); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::fullscreenableChanged, q,
        [window, this] { this->dataChanged(window, IsFullScreenable); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::fullscreenChanged, q,
        [window, this] { this->dataChanged(window, IsFullScreen); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::maximizeableChanged, q,
        [window, this] { this->dataChanged(window, IsMaximizable); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::maximizedChanged, q,
        [window, this] { this->dataChanged(window, IsMaximized); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::minimizeableChanged, q,
        [window, this] { this->dataChanged(window, IsMinimizable); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::minimizedChanged, q,
        [window, this] { this->dataChanged(window, IsMinimized); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::keepAboveChanged, q,
        [window, this] { this->dataChanged(window, IsKeepAbove); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::keepBelowChanged, q,
        [window, this] { this->dataChanged(window, IsKeepBelow); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::shadeableChanged, q,
        [window, this] { this->dataChanged(window, IsShadeable); }
    );

// FIXME
//     QObject::connect(window, &KWayland::Client::PlasmaWindow::virtualDesktopChangeableChanged, q,
//         // TODO: This is marked deprecated in KWayland, but (IMHO) shouldn't be.
//         [window, this] { this->dataChanged(window, IsVirtualDesktopsChangeable); }
//     );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::plasmaVirtualDesktopEntered, q,
        [window, this] {
            this->dataChanged(window, VirtualDesktops);

            // If the count has changed from 0, the window may no longer be on all virtual
            // desktops.
            if (window->plasmaVirtualDesktops().count() > 0) {
                this->dataChanged(window, VirtualDesktops);
            }
        }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::plasmaVirtualDesktopLeft, q,
        [window, this] {
            this->dataChanged(window, VirtualDesktops);

            // If the count has changed to 0, the window is now on all virtual desktops.
            if (window->plasmaVirtualDesktops().count() == 0) {
                this->dataChanged(window, VirtualDesktops);
            }
        }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::geometryChanged, q,
        [window, this] { this->dataChanged(window, QVector<int>{Geometry, ScreenGeometry}); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::demandsAttentionChanged, q,
        [window, this] { this->dataChanged(window, IsDemandingAttention); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::skipTaskbarChanged, q,
        [window, this] { this->dataChanged(window, SkipTaskbar); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::applicationMenuChanged, q,
        [window, this] {
            this->dataChanged(window, QVector<int>{ApplicationMenuServiceName, ApplicationMenuObjectPath});
        }
    );
}

AppData WaylandTasksModel::Private::appData(KWayland::Client::PlasmaWindow *window)
{
    const auto &it = appDataCache.constFind(window);

    if (it != appDataCache.constEnd()) {
        return *it;
    }

    const AppData &data = appDataFromUrl(windowUrlFromMetadata(window->appId(),
        window->pid(), rulesConfig));

    appDataCache.insert(window, data);

    return data;
}

QIcon WaylandTasksModel::Private::icon(KWayland::Client::PlasmaWindow *window)
{
    const AppData &app = appData(window);

    if (!app.icon.isNull()) {
        return app.icon;
    }

    appDataCache[window].icon = window->icon();

    return window->icon();
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

void WaylandTasksModel::Private::dataChanged(KWayland::Client::PlasmaWindow *window, int role)
{
    QModelIndex idx = q->index(windows.indexOf(window));
    emit q->dataChanged(idx, idx, QVector<int>{role});
}

void WaylandTasksModel::Private::dataChanged(KWayland::Client::PlasmaWindow *window, const QVector<int> &roles)
{
    QModelIndex idx = q->index(windows.indexOf(window));
    emit q->dataChanged(idx, idx, roles);
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
    if (!index.isValid() || index.row() >= d->windows.count()) {
        return QVariant();
    }

    KWayland::Client::PlasmaWindow *window = d->windows.at(index.row());

    if (role == Qt::DisplayRole) {
        return window->title();
    } else if (role == Qt::DecorationRole) {
        return d->icon(window);
    } else if (role == AppId) {
        const QString &id = d->appData(window).id;

        if (id.isEmpty()) {
            return window->appId();
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
        return QVariantList() << window->uuid();
    } else if (role == MimeType) {
        return d->mimeType();
    } else if (role == MimeData) {
        return window->uuid();
    } else if (role == IsWindow) {
        return true;
    } else if (role == IsActive) {
        return window->isActive();
    } else if (role == IsClosable) {
        return window->isCloseable();
    } else if (role == IsMovable) {
        return window->isMovable();
    } else if (role == IsResizable) {
        return window->isResizable();
    } else if (role == IsMaximizable) {
        return window->isMaximizeable();
    } else if (role == IsMaximized) {
        return window->isMaximized();
    } else if (role == IsMinimizable) {
        return window->isMinimizeable();
    } else if (role == IsMinimized) {
        return window->isMinimized();
    } else if (role == IsKeepAbove) {
        return window->isKeepAbove();
    } else if (role == IsKeepBelow) {
        return window->isKeepBelow();
    } else if (role == IsFullScreenable) {
        return window->isFullscreenable();
    } else if (role == IsFullScreen) {
        return window->isFullscreen();
    } else if (role == IsShadeable) {
        return window->isShadeable();
    } else if (role == IsShaded) {
        return window->isShaded();
    } else if (role == IsVirtualDesktopsChangeable) {
        // FIXME Currently not implemented in KWayland.
        return true;
    } else if (role == VirtualDesktops) {
        return window->plasmaVirtualDesktops();
    } else if (role == IsOnAllVirtualDesktops) {
        return window->plasmaVirtualDesktops().isEmpty();
    } else if (role == Geometry) {
        return window->geometry();
    } else if (role == ScreenGeometry) {
        return screenGeometry(window->geometry().center());
    } else if (role == Activities) {
        // FIXME Implement.
    } else if (role == IsDemandingAttention) {
        return window->isDemandingAttention();
    } else if (role == SkipTaskbar) {
        return window->skipTaskbar() || d->appData(window).skipTaskbar;
    } else if (role == SkipPager) {
        // FIXME Implement.
    } else if (role == AppPid) {
        return window->pid();
    } else if (role == StackingOrder) {
        return d->windowManagement->stackingOrderUuids().indexOf(window->uuid());
    } else if (role == ApplicationMenuObjectPath) {
        return window->applicationMenuObjectPath();
    } else if (role == ApplicationMenuServiceName) {
        return window->applicationMenuServiceName();
    }

    return QVariant();
}

int WaylandTasksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->windows.count();
}

QModelIndex WaylandTasksModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column, d->windows.at(row)) : QModelIndex();
}

void WaylandTasksModel::requestActivate(const QModelIndex &index)
{
    // FIXME Lacks transient handling of the XWindows version.

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestActivate();
}

void WaylandTasksModel::requestNewInstance(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    runApp(d->appData(d->windows.at(index.row())));
}

void WaylandTasksModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    if (!index.isValid() || index.model() != this || index.row() < 0
        || index.row() >= d->windows.count()
        || urls.isEmpty()) {
        return;
    }

    runApp(d->appData(d->windows.at(index.row())), urls);
}

void WaylandTasksModel::requestClose(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestClose();
}

void WaylandTasksModel::requestMove(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    KWayland::Client::PlasmaWindow *window = d->windows.at(index.row());

    const QString &currentDesktop = d->virtualDesktopInfo->currentDesktop().toString();

    if (!currentDesktop.isEmpty()) {
        window->requestEnterVirtualDesktop(currentDesktop);
    }

    window->requestMove();
}

void WaylandTasksModel::requestResize(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    KWayland::Client::PlasmaWindow *window = d->windows.at(index.row());

    const QString &currentDesktop = d->virtualDesktopInfo->currentDesktop().toString();

    if (!currentDesktop.isEmpty()) {
        window->requestEnterVirtualDesktop(currentDesktop);
    }

    window->requestResize();
}

void WaylandTasksModel::requestToggleMinimized(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    KWayland::Client::PlasmaWindow *window = d->windows.at(index.row());

    const QString &currentDesktop = d->virtualDesktopInfo->currentDesktop().toString();

    if (!currentDesktop.isEmpty()) {
        window->requestEnterVirtualDesktop(currentDesktop);
    }

    window->requestToggleMinimized();
}

void WaylandTasksModel::requestToggleMaximized(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    KWayland::Client::PlasmaWindow *window = d->windows.at(index.row());

    const QString &currentDesktop = d->virtualDesktopInfo->currentDesktop().toString();

    if (!currentDesktop.isEmpty()) {
        window->requestEnterVirtualDesktop(currentDesktop);
    }

    window->requestToggleMaximized();
}

void WaylandTasksModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestToggleKeepAbove();
}

void WaylandTasksModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestToggleKeepBelow();
}

void WaylandTasksModel::requestToggleFullScreen(const QModelIndex &index)
{
    Q_UNUSED(index)

    // FIXME Implement.
}

void WaylandTasksModel::requestToggleShaded(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestToggleShaded();
}

void WaylandTasksModel::requestVirtualDesktops(const QModelIndex &index, const QVariantList &desktops)
{
    // FIXME TODO: Lacks the "if we've requested the current desktop, force-activate
    // the window" logic from X11 version. This behavior should be in KWin rather than
    // libtm however.

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    KWayland::Client::PlasmaWindow *window = d->windows.at(index.row());

    if (desktops.isEmpty()) {
        foreach (const QVariant &desktop, window->plasmaVirtualDesktops()) {
            window->requestLeaveVirtualDesktop(desktop.toString());
        }
    } else {
        const QStringList &now = window->plasmaVirtualDesktops();
        QStringList next;

        foreach (const QVariant &desktop, desktops) {
            const QString &desktopId = desktop.toString();

            if (!desktopId.isEmpty()) {
                next << desktopId;

                if (!now.contains(desktopId)) {
                    window->requestEnterVirtualDesktop(desktopId);
                }
            }
        }

        foreach (const QString &desktop, now) {
            if (!next.contains(desktop)) {
                window->requestLeaveVirtualDesktop(desktop);
            }
        }
    }
}

void WaylandTasksModel::requestNewVirtualDesktop(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestEnterNewVirtualDesktop();
}

void WaylandTasksModel::requestActivities(const QModelIndex &index, const QStringList &activities)
{
    Q_UNUSED(index)
    Q_UNUSED(activities)
}

void WaylandTasksModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    /*
    FIXME: This introduces the dependency on Qt5::Quick. I might prefer
    reversing this and publishing the window pointer through the model,
    then calling PlasmaWindow::setMinimizeGeometry in the applet backend,
    rather than hand delegate items into the lib, keeping the lib more UI-
    agnostic.
    */

    Q_UNUSED(geometry)

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    const QQuickItem *item = qobject_cast<const QQuickItem *>(delegate);

    if (!item || !item->parentItem() || !item->window()) {
        return;
    }

    QWindow *itemWindow = item->window();

    if (!itemWindow) {
        return;
    }

    using namespace KWayland::Client;
    Surface *surface = Surface::fromWindow(itemWindow);

    if (!surface) {
        return;
    }

    QRect rect(item->x(), item->y(), item->width(), item->height());
    rect.moveTopLeft(item->parentItem()->mapToScene(rect.topLeft()).toPoint());

    KWayland::Client::PlasmaWindow *window = d->windows.at(index.row());

    window->setMinimizedGeometry(surface, rect);
}

quint32 WaylandTasksModel::winIdFromMimeData(const QMimeData *mimeData, bool *ok)
{
    Q_ASSERT(mimeData);

    if (ok) {
        *ok = false;
    }

    if (!mimeData->hasFormat(Private::mimeType())) {
        return 0;
    }

    quint32 id = mimeData->data(Private::mimeType()).toUInt(ok);

    return id;
}

QList<quint32> WaylandTasksModel::winIdsFromMimeData(const QMimeData *mimeData, bool *ok)
{
    Q_ASSERT(mimeData);
    QList<quint32> ids;

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

    // FIXME: Extracting multiple winids is still unimplemented;
    // TaskGroupingProxy::data(..., ::MimeData) can't produce
    // a payload with them anyways.

    return ids;
}

}
