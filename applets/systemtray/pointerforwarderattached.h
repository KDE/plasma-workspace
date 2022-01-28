/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <QQuickItem>
#include <QQmlListProperty>

/**
 * An attached object that takes pointer events from its parent Item and
 * forwards them to other Items.
 */
class PointerForwarderAttached : public QObject
{
    Q_OBJECT

    /**
     * This property holds the target item to forward pointer events to.
     *
     * Pointer events are not sent directly to the target item because that
     * wouldn't be the most useful way to handle events a lot of the time,
     * unless you intend to only use an item that directly uses pointer events.
     * Instead, copies of events are made with their positions changed to be
     * within the bounds of the target item, relative to the original positions.
     * The copies are sent to the window so that the events can be processed
     * like they normally would be by items stacked above the target item.
     * For example, if you have a target item that has a MouseArea stacked over
     * it, the MouseArea would get the events first, then the target item.
     */
    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged FINAL)

    /**
     * This property holds whether the PointerForwarder is able to forward events.
     */
    Q_PROPERTY(bool enabled MEMBER enabled NOTIFY enabledChanged FINAL)

public:
    explicit PointerForwarderAttached(QObject *parent = nullptr);
    ~PointerForwarderAttached() override;

    QQuickItem *target() const;
    void setTarget(QQuickItem *item);

    bool eventFilter(QObject *watched, QEvent *event) override;

    static PointerForwarderAttached *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void targetChanged();
    void enabledChanged();

private:
    QPointF movePointToRect(QPointF pos, const QRectF &rect) const;
    bool forwardEvent(QEvent *event);

    QQuickItem *parentItem;
    QQuickItem *targetItem;
    bool enabled;
    // Keeps track of the events we send to prevent loops.
    QList<QEvent *> sentEvents;
};

QML_DECLARE_TYPEINFO(PointerForwarderAttached, QML_HAS_ATTACHED_PROPERTIES)
