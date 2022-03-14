/*
    SPDX-FileCopyrightText: 2013 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "draghelper.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

DragHelper::DragHelper(QObject *parent)
    : QObject(parent)
{
}

DragHelper::~DragHelper()
{
}

int DragHelper::dragIconSize() const
{
    return m_dragIconSize;
}

void DragHelper::setDragIconSize(int size)
{
    if (m_dragIconSize != size) {
        m_dragIconSize = size;

        Q_EMIT dragIconSizeChanged();
    }
}

bool DragHelper::isDrag(int oldX, int oldY, int newX, int newY) const
{
    return ((QPoint(oldX, oldY) - QPoint(newX, newY)).manhattanLength() >= QApplication::startDragDistance());
}

void DragHelper::startDrag(QQuickItem *item, const QUrl &url, const QIcon &icon, const QString &extraMimeType, const QString &extraMimeData)
{
    // This allows the caller to return, making sure we don't crash if
    // the caller is destroyed mid-drag (as can happen due to a sycoca
    // change).

    QMetaObject::invokeMethod(this,
                              "doDrag",
                              Qt::QueuedConnection,
                              Q_ARG(QQuickItem *, item),
                              Q_ARG(QUrl, url),
                              Q_ARG(QIcon, icon),
                              Q_ARG(QString, extraMimeType),
                              Q_ARG(QString, extraMimeData));
}

void DragHelper::doDrag(QQuickItem *item, const QUrl &url, const QIcon &icon, const QString &extraMimeType, const QString &extraMimeData)
{
    setDragging(true);

    if (item && item->window() && item->window()->mouseGrabberItem()) {
        item->window()->mouseGrabberItem()->ungrabMouse();
    }

    QDrag *drag = new QDrag(item);

    QMimeData *mimeData = new QMimeData();

    if (!url.isEmpty()) {
        mimeData->setUrls(QList<QUrl>() << url);
    }

    if (!extraMimeType.isEmpty() && !extraMimeData.isEmpty()) {
        mimeData->setData(extraMimeType, extraMimeData.toLatin1());
    }

    drag->setMimeData(mimeData);

    if (!icon.isNull()) {
        drag->setPixmap(icon.pixmap(m_dragIconSize, m_dragIconSize));
    }

    drag->exec();

    Q_EMIT dropped();

    // Ensure dragging is still true when onRelease is called.
    QTimer::singleShot(0, qApp, [this] {
        setDragging(false);
    });
}

void DragHelper::setDragging(bool dragging)
{
    if (m_dragging == dragging)
        return;
    m_dragging = dragging;
    Q_EMIT draggingChanged();
}
