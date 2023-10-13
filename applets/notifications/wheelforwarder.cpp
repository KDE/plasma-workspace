/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "wheelforwarder.h"

#include <QCoreApplication>
#include <QQuickItem>

void WheelForwarder::interceptWheelEvent(QQuickItem *from)
{
    from->installEventFilter(this);
}

bool WheelForwarder::eventFilter(QObject *, QEvent *event)
{
    if (event->type() != QEvent::Wheel) {
        return false;
    }
    QCoreApplication::sendEvent(m_toItem, event);
    return true;
}
