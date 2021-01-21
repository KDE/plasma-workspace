/*
 *   Copyright 2019 by Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#pragma once

#include <QQuickItem>
#include <QTimer>

#include "itemcontainer.h"

class ConfigOverlay : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool open READ open WRITE setOpen NOTIFY openChanged)
    Q_PROPERTY(ItemContainer *itemContainer READ itemContainer NOTIFY itemContainerChanged)
    Q_PROPERTY(qreal leftAvailableSpace READ leftAvailableSpace NOTIFY leftAvailableSpaceChanged);
    Q_PROPERTY(qreal topAvailableSpace READ topAvailableSpace NOTIFY topAvailableSpaceChanged);
    Q_PROPERTY(qreal rightAvailableSpace READ rightAvailableSpace NOTIFY rightAvailableSpaceChanged);
    Q_PROPERTY(qreal bottomAvailableSpace READ bottomAvailableSpace NOTIFY bottomAvailableSpaceChanged);
    Q_PROPERTY(bool touchInteraction READ touchInteraction NOTIFY touchInteractionChanged)

public:
    ConfigOverlay(QQuickItem *parent = nullptr);
    ~ConfigOverlay();

    ItemContainer *itemContainer() const;
    // NOTE: setter not accessible from QML by purpose
    void setItemContainer(ItemContainer *container);

    bool open() const;
    void setOpen(bool open);

    qreal leftAvailableSpace()
    {
        return m_leftAvailableSpace;
    }
    qreal topAvailableSpace()
    {
        return m_topAvailableSpace;
    }
    qreal rightAvailableSpace()
    {
        return m_rightAvailableSpace;
    }
    qreal bottomAvailableSpace()
    {
        return m_bottomAvailableSpace;
    }

    bool touchInteraction() const;
    // This only usable from C++
    void setTouchInteraction(bool touch);

Q_SIGNALS:
    void openChanged();
    void itemContainerChanged();
    void leftAvailableSpaceChanged();
    void topAvailableSpaceChanged();
    void rightAvailableSpaceChanged();
    void bottomAvailableSpaceChanged();
    void touchInteractionChanged();

private:
    QPointer<ItemContainer> m_itemContainer;
    qreal m_leftAvailableSpace = 0;
    qreal m_topAvailableSpace = 0;
    qreal m_rightAvailableSpace = 0;
    qreal m_bottomAvailableSpace = 0;

    QTimer *m_hideTimer = nullptr;

    QList<QTouchEvent::TouchPoint> m_oldTouchPoints;

    bool m_open = false;
    bool m_touchInteraction = false;
};
