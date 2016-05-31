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

#include "concatenatetasksproxymodel.h"

namespace TaskManager
{

ConcatenateTasksProxyModel::ConcatenateTasksProxyModel(QObject *parent)
    : KConcatenateRowsProxyModel(parent)
{
}

ConcatenateTasksProxyModel::~ConcatenateTasksProxyModel()
{
}

void ConcatenateTasksProxyModel::requestActivate(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestActivate(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestNewInstance(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestNewInstance(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestClose(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestClose(sourceIndex);
    }
}
void ConcatenateTasksProxyModel::requestMove(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestMove(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestResize(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestResize(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestToggleMinimized(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestToggleMinimized(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestToggleMaximized(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestToggleMaximized(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestToggleKeepAbove(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

   const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestToggleKeepBelow(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestToggleFullScreen(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestToggleFullScreen(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestToggleShaded(const QModelIndex &index)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestToggleShaded(sourceIndex);
    }
}

void ConcatenateTasksProxyModel::requestVirtualDesktop(const QModelIndex &index, qint32 desktop)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestVirtualDesktop(sourceIndex, desktop);
    }
}

void ConcatenateTasksProxyModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    if (!index.isValid() || index.model() != this) {
        return;
    }

    const QModelIndex &sourceIndex = mapToSource(index);
    const AbstractTasksModelIface *m = dynamic_cast<const AbstractTasksModelIface *>(sourceIndex.model());

    if (m) {
        // NOTE: KConcatenateRowsProxyModel offers no way to get a non-const pointer
        // to one of the source models, so we have to go through a mapped index.
        const_cast<AbstractTasksModelIface *>(m)->requestPublishDelegateGeometry(sourceIndex, geometry, delegate);
    }
}

}
