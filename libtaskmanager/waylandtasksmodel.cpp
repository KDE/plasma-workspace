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

#include <KActivities/ResourceInstance>
#include <KRun>
#include <KService>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>

#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QUrl>
#include <QWindow>

namespace TaskManager
{

class WaylandTasksModel::Private
{
public:
    Private(WaylandTasksModel *q);
    QList<KWayland::Client::PlasmaWindow*> windows;
    QHash<KWayland::Client::PlasmaWindow*, KService::Ptr> serviceCache;
    KWayland::Client::PlasmaWindowManagement *windowManagement = nullptr;

    void initWayland();
    void addWindow(KWayland::Client::PlasmaWindow *window);
    void dataChanged(KWayland::Client::PlasmaWindow *window, int role);
    void dataChanged(KWayland::Client::PlasmaWindow *window, const QVector<int> &roles);

private:
    WaylandTasksModel *q;
};

WaylandTasksModel::Private::Private(WaylandTasksModel *q)
    : q(q)
{
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

    QObject::connect(registry, &KWayland::Client::Registry::plasmaWindowManagementAnnounced, [this, registry] (quint32 name, quint32 version) {
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

    KService::Ptr service = KService::serviceByStorageId(window->appId());

    if (service) {
        serviceCache.insert(window, service);
    }

    q->endInsertRows();

    auto removeWindow = [window, this] {
        const int row = windows.indexOf(window);
        if (row != -1) {
            q->beginRemoveRows(QModelIndex(), row, row);
            windows.removeAt(row);
            serviceCache.remove(window);
            q->endRemoveRows();
        }
    };

    QObject::connect(window, &KWayland::Client::PlasmaWindow::unmapped, q, removeWindow);
    QObject::connect(window, &QObject::destroyed, q, removeWindow);

    QObject::connect(window, &KWayland::Client::PlasmaWindow::titleChanged, q,
        [window, this] { dataChanged(window, Qt::DisplayRole); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::iconChanged, q,
        [window, this] { dataChanged(window, Qt::DecorationRole); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::appIdChanged, q,
        [window, this] {
            KService::Ptr service = KService::serviceByStorageId(window->appId());

            if (service) {
                serviceCache.insert(window, service);
            } else {
                serviceCache.remove(window);
            }

            dataChanged(window, QVector<int>{AppId, AppName, GenericName,
                LauncherUrl, LauncherUrlWithoutIcon});
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

    QObject::connect(window, &KWayland::Client::PlasmaWindow::virtualDesktopChangeableChanged, q,
        [window, this] { this->dataChanged(window, IsVirtualDesktopChangeable); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::virtualDesktopChanged, q,
        [window, this] { this->dataChanged(window, VirtualDesktop); }
    );

    QObject::connect(window, &KWayland::Client::PlasmaWindow::onAllDesktopsChanged, q,
        [window, this] { this->dataChanged(window, IsOnAllVirtualDesktops); }
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
    d->initWayland();
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
        return window->icon();
    } else if (role == AppId) {
        return window->appId();
    } else if (role == AppName) {
        if (d->serviceCache.contains(window)) {
            return d->serviceCache.value(window)->name();
        } else {
            return window->title();
        }
    } else if (role == GenericName) {
        if (d->serviceCache.contains(window)) {
            return d->serviceCache.value(window)->genericName();
        }
    } else if (role == LauncherUrl || role == LauncherUrlWithoutIcon) {
        if (d->serviceCache.contains(window)) {
            return QUrl::fromLocalFile(d->serviceCache.value(window)->entryPath());
        }
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
    } else if (role == IsVirtualDesktopChangeable) {
        return window->isVirtualDesktopChangeable();
    } else if (role == VirtualDesktop) {
        return window->virtualDesktop();
    } else if (role == IsOnAllVirtualDesktops) {
        return window->isOnAllDesktops();
    } else if (role == Geometry) {
        return window->geometry();
    } else if (role == ScreenGeometry) {
        return screenGeometry(window->geometry().center());
    } else if (role == Activities) {
        // FIXME Implement.
    } else if (role == IsDemandingAttention) {
        return window->isDemandingAttention();
    } else if (role == SkipTaskbar) {
        return window->skipTaskbar();
    } else if (role == SkipPager) {
        // FIXME Implement.
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

    KWayland::Client::PlasmaWindow* window = d->windows.at(index.row());

    if (d->serviceCache.contains(window)) {
        const KService::Ptr service = d->serviceCache.value(window);

        new KRun(QUrl::fromLocalFile(service->entryPath()), 0, false);

        KActivities::ResourceInstance::notifyAccessed(QUrl("applications:" + service->storageId()),
            "org.kde.libtaskmanager");
    }
}

void WaylandTasksModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    if (!index.isValid() || index.model() != this || index.row() < 0
        || index.row() >= d->windows.count()
        || urls.isEmpty()) {
        return;
    }

    KWayland::Client::PlasmaWindow *window = d->windows.at(index.row());

    if (d->serviceCache.contains(window)) {
        const KService::Ptr service = d->serviceCache.value(window);

        KRun::runApplication(*service, urls, nullptr, 0);

        KActivities::ResourceInstance::notifyAccessed(QUrl("applications:" + service->storageId()),
            "org.kde.libtaskmanager");
    }
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
    // FIXME Move-to-desktop logic from XWindows version. (See also others.)

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestMove();
}

void WaylandTasksModel::requestResize(const QModelIndex &index)
{
    // FIXME Move-to-desktop logic from XWindows version. (See also others.)

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestResize();
}

void WaylandTasksModel::requestToggleMinimized(const QModelIndex &index)
{
    // FIXME Move-to-desktop logic from XWindows version. (See also others.)

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestToggleMinimized();
}

void WaylandTasksModel::requestToggleMaximized(const QModelIndex &index)
{
    // FIXME Move-to-desktop logic from XWindows version. (See also others.)

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestToggleMaximized();
}

void WaylandTasksModel::requestToggleKeepAbove(const QModelIndex &index)
{
    Q_UNUSED(index)

    // FIXME Implement.
}

void WaylandTasksModel::requestToggleKeepBelow(const QModelIndex &index)
{
    Q_UNUSED(index)

    // FIXME Implement.
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

void WaylandTasksModel::requestVirtualDesktop(const QModelIndex &index, qint32 desktop)
{
    // FIXME Lacks add-new-desktop code from XWindows version.
    // FIXME Does this do the set-on-all-desktops stuff from the XWindows version?

    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= d->windows.count()) {
        return;
    }

    d->windows.at(index.row())->requestVirtualDesktop(desktop);
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

}
