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

#include "configoverlay.h"

class ResizeHandle : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Corner resizeCorner MEMBER m_resizeCorner NOTIFY resizeCornerChanged)
    Q_PROPERTY(bool resizeBlocked READ resizeBlocked NOTIFY resizeBlockedChanged)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)

public:
    enum Corner {
        Left = 0,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
    };
    Q_ENUMS(Corner)

    ResizeHandle(QQuickItem *parent = nullptr);
    ~ResizeHandle();

    bool resizeBlocked() const;

    void setPressed(bool pressed);
    bool isPressed() const;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;

Q_SIGNALS:
    void resizeCornerChanged();
    void resizeBlockedChanged();
    void pressedChanged();

private:
    void setConfigOverlay(ConfigOverlay *configOverlay);

    inline bool resizeLeft() const;
    inline bool resizeTop() const;
    inline bool resizeRight() const;
    inline bool resizeBottom() const;
    void setResizeBlocked(bool width, bool height);

    QPointF m_mouseDownPosition;
    QRectF m_mouseDownGeometry;

    QPointer<ConfigOverlay> m_configOverlay;
    Corner m_resizeCorner = Left;
    bool m_resizeWidthBlocked = false;
    bool m_resizeHeightBlocked = false;
    bool m_pressed = false;
};
