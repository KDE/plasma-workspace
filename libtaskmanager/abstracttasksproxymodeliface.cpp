/********************************************************************
Copyright 2016  David Edmundson <davidedmundson@kde.org>

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

#include "abstracttasksproxymodeliface.h"

#include <QAbstractItemModel>
#include <QModelIndex>

namespace TaskManager
{
void AbstractTasksProxyModelIface::requestActivate(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestActivate(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestNewInstance(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestNewInstance(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestOpenUrls(sourceIndex, urls);
    }
}

void AbstractTasksProxyModelIface::requestClose(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestClose(sourceIndex);
    }
}
void AbstractTasksProxyModelIface::requestMove(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestMove(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestResize(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestResize(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestToggleMinimized(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestToggleMinimized(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestToggleMaximized(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestToggleMaximized(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestToggleKeepAbove(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestToggleKeepAbove(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestToggleKeepBelow(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestToggleKeepBelow(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestToggleFullScreen(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestToggleFullScreen(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestToggleShaded(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestToggleShaded(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestVirtualDesktops(const QModelIndex &index, const QVariantList &desktops)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestVirtualDesktops(sourceIndex, desktops);
    }
}

void AbstractTasksProxyModelIface::requestNewVirtualDesktop(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestNewVirtualDesktop(sourceIndex);
    }
}

void AbstractTasksProxyModelIface::requestActivities(const QModelIndex &index, const QStringList &activities)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestActivities(sourceIndex, activities);
    }
}

void AbstractTasksProxyModelIface::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex &sourceIndex = mapIfaceToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        const_cast<AbstractTasksModelIface *>(m)->requestPublishDelegateGeometry(sourceIndex, geometry, delegate);
    }
}

}
