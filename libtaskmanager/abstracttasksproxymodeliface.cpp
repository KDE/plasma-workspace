/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

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
