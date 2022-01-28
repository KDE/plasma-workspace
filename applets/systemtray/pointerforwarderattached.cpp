/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "pointerforwarderattached.h"
#include <QtQml>
#include <QCoreApplication>
#include <QQuickWindow>

// TODO Qt6: replace all of these event classes with just QPointerEvent

// following are QSinglePointerEvent subclasses in Qt6:
// - QEnterEvent, not handled in this class since that isn't really used in Qt Quick
#include <QHoverEvent>
#include <QMouseEvent>
// - QNativeGestureEvent, not handled in this class for now since it's only used by PinchArea and PinchHandler
#include <QTabletEvent>
#include <QWheelEvent>

// Not a QSinglePointerEvent subclass
#include <QTouchEvent>

PointerForwarderAttached::PointerForwarderAttached(QObject *parent)
    : QObject(parent)
    , parentItem(qobject_cast<QQuickItem *>(parent))
    , targetItem(nullptr)
    , enabled(true)
{
    if (parentItem) {
        parentItem->installEventFilter(this);
    } else {
        qmlWarning(this) << "Could not install PointerForwarder event filter: parent is not an Item";
    }
}

PointerForwarderAttached::~PointerForwarderAttached() = default;

QQuickItem* PointerForwarderAttached::target() const
{
    return targetItem;
}

void PointerForwarderAttached::setTarget(QQuickItem *item)
{
    if (item == targetItem) {
        return;
    }
    if (item == parentItem) {
        qmlWarning(this) << "Invalid target: parent is the same as target. This could cause a loop.";
        return;
    }
    targetItem = item;
    Q_EMIT targetChanged();
}

QPointF PointerForwarderAttached::movePointToRect(QPointF pos, const QRectF &rect) const
{
    if (rect.isEmpty()) {
        qmlWarning(this) << "Cannot move event position to target: target has empty size";
        return pos;
    }
    // A crude way to move a point to the bounds of the rectangle
    if (pos.x() < rect.left()) { // west of rect
        pos.setX(rect.left());
    } else if (pos.x() > rect.right()) { // east of rect
        pos.setX(rect.right());
    }
    if (pos.y() < rect.top()) { // north of rect
        pos.setY(rect.top());
    } else if (pos.y() > rect.bottom()) { // south of rect
        pos.setY(rect.bottom());
    }
    return pos;
}

bool PointerForwarderAttached::forwardEvent(QEvent *event)
{
    if (event == nullptr || parentItem == nullptr || targetItem == nullptr
        || !targetItem->isVisible() || !targetItem->isEnabled()
        || targetItem->window() == nullptr) {
        return false;
    }
    // some kinds of events such as double click come with other events,
    // so we need a list to prevent different events coming through from causing a loop.
    while (sentEvents.length() > 2) {
        sentEvents.removeFirst();
    }
    sentEvents.append(event);
    QCoreApplication::postEvent(targetItem->window(), event);
    return true;
}

bool PointerForwarderAttached::eventFilter(QObject *watched, QEvent *event)
{
    if (!enabled || watched == nullptr || event == nullptr || targetItem == nullptr
        || sentEvents.contains(event) // Skip to prevent possible loop
    ) {
        qDebug() << this << event;
        return false;
    }
    Q_ASSERT(watched == parentItem);

    QRectF targetLocalRect = {targetItem->position(), targetItem->size()};
    QRectF targetSceneRect = targetItem->mapRectToScene(targetLocalRect);

    QEvent *newEvent = nullptr;
    // TODO Qt6: Replace all these with just QPointerEvent. It would simplify the code a lot.
    if (auto hoverEvent = dynamic_cast<QHoverEvent *>(event)) {
        QPointF pos = hoverEvent->posF();
        QPointF oldPos = hoverEvent->oldPosF();
        // HoverLeave should have (-1,-1)
        if (hoverEvent->type() != QEvent::HoverLeave) {
            pos = parentItem->mapToItem(targetItem, pos);
            pos = movePointToRect(pos, targetLocalRect);
        }
        // HoverEnter should have (-1,-1)
        if (hoverEvent->type() != QEvent::HoverEnter) {
            oldPos = parentItem->mapToItem(targetItem, oldPos);
            oldPos = movePointToRect(oldPos, targetLocalRect);
        }

        newEvent = new QHoverEvent(event->type(), pos, oldPos, hoverEvent->modifiers());

    } else if (auto mouseEvent = dynamic_cast<QMouseEvent *>(event)) {
        QPointF localPos = parentItem->mapToItem(targetItem, mouseEvent->localPos());
        localPos = movePointToRect(localPos, targetLocalRect);

        newEvent = new QMouseEvent(event->type(), localPos, mouseEvent->windowPos(), mouseEvent->screenPos(), mouseEvent->button(),
                                   mouseEvent->buttons(), mouseEvent->modifiers(), mouseEvent->source());

    } else if (auto tabletEvent = dynamic_cast<QTabletEvent *>(event)) {
        QPointF pos = parentItem->mapToItem(targetItem, tabletEvent->posF());
        pos = movePointToRect(pos, targetLocalRect);

        newEvent = new QTabletEvent(event->type(), pos, tabletEvent->globalPosF(), tabletEvent->deviceType(),
                                    tabletEvent->pointerType(), tabletEvent->pressure(), tabletEvent->xTilt(),
                                    tabletEvent->yTilt(), tabletEvent->tangentialPressure(), tabletEvent->rotation(),
                                    tabletEvent->z(), tabletEvent->modifiers(), tabletEvent->uniqueId(),
                                    tabletEvent->button(), tabletEvent->buttons());

    } else if (auto wheelEvent = dynamic_cast<QWheelEvent *>(event)) {
        QPointF position = parentItem->mapToItem(targetItem, wheelEvent->position());
        position = movePointToRect(position, targetLocalRect);

        newEvent = new QWheelEvent(position, wheelEvent->globalPosition(), wheelEvent->pixelDelta(),
                                   wheelEvent->angleDelta(), wheelEvent->buttons(), wheelEvent->modifiers(),
                                   wheelEvent->phase(), wheelEvent->inverted(), wheelEvent->source());

    } else if (auto touchEvent = dynamic_cast<QTouchEvent *>(event)) {
        // Don't bother with multi-touch events. Too complex to deal with for common situations.
        if (touchEvent->touchPoints().length() != 1) {
            return false;
        }
        QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
        QTouchEvent::TouchPoint touchPoint = touchPoints.at(0);
        // in local coordinates, must map
        QPointF pos = parentItem->mapToItem(targetItem, touchPoint.pos());
        touchPoint.setPos(movePointToRect(pos, targetLocalRect));
        // already in scene coordinates, no need to map
        touchPoint.setScenePos(movePointToRect(touchPoint.scenePos(), targetSceneRect));
        touchPoints[0] = touchPoint;

        newEvent = new QTouchEvent(event->type(), touchEvent->device(), touchEvent->modifiers(),
                                   touchEvent->touchPointStates(), touchPoints);

    }
    qDebug() << newEvent << targetItem;
    return forwardEvent(newEvent);
}

PointerForwarderAttached *PointerForwarderAttached::qmlAttachedProperties(QObject *object)
{
    return new PointerForwarderAttached(object);
}
