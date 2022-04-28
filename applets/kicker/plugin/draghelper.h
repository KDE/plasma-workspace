/*
    SPDX-FileCopyrightText: 2013 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QIcon>
#include <QObject>
#include <QUrl>

class QQuickItem;

class DragHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int dragIconSize READ dragIconSize WRITE setDragIconSize NOTIFY dragIconSizeChanged)
    Q_PROPERTY(bool dragging READ isDragging NOTIFY draggingChanged)

public:
    explicit DragHelper(QObject *parent = nullptr);

    int dragIconSize() const;
    void setDragIconSize(int size);
    bool isDragging() const
    {
        return m_dragging;
    }

    Q_INVOKABLE bool isDrag(int oldX, int oldY, int newX, int newY) const;
    Q_INVOKABLE void startDrag(QQuickItem *item,
                               const QUrl &url = QUrl(),
                               const QIcon &icon = QIcon(),
                               const QString &extraMimeType = QString(),
                               const QString &extraMimeData = QString());

Q_SIGNALS:
    void dragIconSizeChanged() const;
    void dropped() const;
    void draggingChanged() const;

private:
    int m_dragIconSize = 32;
    bool m_dragging = false;
    Q_INVOKABLE void doDrag(QQuickItem *item,
                            const QUrl &url = QUrl(),
                            const QIcon &icon = QIcon(),
                            const QString &extraMimeType = QString(),
                            const QString &extraMimeData = QString());
    void setDragging(bool dragging);
};
