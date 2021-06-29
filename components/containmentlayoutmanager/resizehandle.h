/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
