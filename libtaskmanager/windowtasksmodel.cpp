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

#include "windowtasksmodel.h"

#include <config-X11.h>

#include "waylandtasksmodel.h"
#if HAVE_X11
#include "xwindowtasksmodel.h"
#endif

#include <KWindowSystem>

namespace TaskManager
{

class WindowTasksModel::Private
{
public:
    Private(WindowTasksModel *q);
    ~Private();

    static int instanceCount;
    static AbstractTasksModel* sourceTasksModel;

    void initSourceTasksModel();

private:
    WindowTasksModel *q;
};

int WindowTasksModel::Private::instanceCount = 0;
AbstractTasksModel* WindowTasksModel::Private::sourceTasksModel = nullptr;

WindowTasksModel::Private::Private(WindowTasksModel *q)
    : q(q)
{
    ++instanceCount;
}

WindowTasksModel::Private::~Private()
{
    --instanceCount;


    if (!instanceCount) {
        delete sourceTasksModel;
        sourceTasksModel = nullptr;
    }
}

void WindowTasksModel::Private::initSourceTasksModel()
{
    if (!sourceTasksModel && KWindowSystem::isPlatformWayland()) {
        sourceTasksModel = new WaylandTasksModel();
    }

#if HAVE_X11
    if (!sourceTasksModel && KWindowSystem::isPlatformX11()) {
        sourceTasksModel = new XWindowTasksModel();
    }
#endif

    q->setSourceModel(sourceTasksModel);
}

WindowTasksModel::WindowTasksModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , d(new Private(this))
{
    d->initSourceTasksModel();
}

WindowTasksModel::~WindowTasksModel()
{
}

QHash<int, QByteArray> WindowTasksModel::roleNames() const
{
    if (d->sourceTasksModel) {
        return d->sourceTasksModel->roleNames();
    }

    return QHash<int, QByteArray>();
}

void WindowTasksModel::requestActivate(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestActivate(mapToSource(index));
    }
}

void WindowTasksModel::requestNewInstance(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestNewInstance(mapToSource(index));
    }
}

void WindowTasksModel::requestClose(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestClose(mapToSource(index));
    }
}

void WindowTasksModel::requestMove(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestMove(mapToSource(index));
    }
}

void WindowTasksModel::requestResize(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestResize(mapToSource(index));
    }
}

void WindowTasksModel::requestToggleMinimized(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleMinimized(mapToSource(index));
    }
}

void WindowTasksModel::requestToggleMaximized(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleMaximized(mapToSource(index));
    }
}

void WindowTasksModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleKeepAbove(mapToSource(index));
    }
}

void WindowTasksModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleKeepBelow(mapToSource(index));
    }
}

void WindowTasksModel::requestToggleFullScreen(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleFullScreen(mapToSource(index));
    }
}

void WindowTasksModel::requestToggleShaded(const QModelIndex &index)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestToggleShaded(mapToSource(index));
    }
}

void WindowTasksModel::requestVirtualDesktop(const QModelIndex &index, qint32 desktop)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestVirtualDesktop(mapToSource(index), desktop);
    }
}

void WindowTasksModel::requestActivities(const QModelIndex &index, const QStringList &activities)
{
    if (d->sourceTasksModel && index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestActivities(mapToSource(index), activities);
    }
}

void WindowTasksModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    if (index.isValid() && index.model() == this) {
        d->sourceTasksModel->requestPublishDelegateGeometry(mapToSource(index), geometry, delegate);
    }
}

}
