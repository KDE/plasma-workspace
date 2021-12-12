/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "wheelinterceptor.h"

#include <QCoreApplication>

WheelInterceptor::WheelInterceptor(QQuickItem *parent)
    : QQuickItem(parent)
{
}

WheelInterceptor::~WheelInterceptor()
{
}

QQuickItem *WheelInterceptor::destination() const
{
    return m_destination;
}

void WheelInterceptor::setDestination(QQuickItem *destination)
{
    if (m_destination != destination) {
        m_destination = destination;

        Q_EMIT destinationChanged();
    }
}

void WheelInterceptor::wheelEvent(QWheelEvent *event)
{
    if (m_destination) {
        QCoreApplication::sendEvent(m_destination, event);
    }

    Q_EMIT wheelMoved(event->angleDelta());
}

QQuickItem *WheelInterceptor::findWheelArea(QQuickItem *parent) const
{
    if (!parent) {
        return nullptr;
    }

    const QList<QQuickItem *> childItems = parent->childItems();
    for (QQuickItem *child : childItems) {
        // HACK: ScrollView adds the WheelArea below its flickableItem with
        // z==-1. This is reasonable non-risky considering we know about
        // everything else in there, and worst case we break the mouse wheel.
        if (child->z() == -1) {
            return child;
        }
    }

    return nullptr;
}
