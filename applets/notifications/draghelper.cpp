/*
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "draghelper.h"

#include <QDrag>
#include <QGuiApplication>
#include <QMetaObject>
#include <QMimeData>
#include <QQuickWindow>
#include <QStyleHints>

DragHelper::DragHelper(QObject *parent)
    : QObject(parent)
{
}

DragHelper::~DragHelper() = default;

bool DragHelper::dragActive() const
{
    return m_dragActive;
}

int DragHelper::dragPixmapSize() const
{
    return m_dragPixmapSize;
}

void DragHelper::setDragPixmapSize(int dragPixmapSize)
{
    if (m_dragPixmapSize != dragPixmapSize) {
        m_dragPixmapSize = dragPixmapSize;
        Q_EMIT dragPixmapSizeChanged();
    }
}

bool DragHelper::isDrag(int oldX, int oldY, int newX, int newY) const
{
    return ((QPoint(oldX, oldY) - QPoint(newX, newY)).manhattanLength() >= qApp->styleHints()->startDragDistance());
}

void DragHelper::startDrag(QQuickItem *item, const QUrl &url, const QString &iconName)
{
    startDrag(item, url, QIcon::fromTheme(iconName).pixmap(m_dragPixmapSize, m_dragPixmapSize));
}

void DragHelper::startDrag(QQuickItem *item, const QUrl &url, const QPixmap &pixmap)
{
    // This allows the caller to return, making sure we don't crash if
    // the caller is destroyed mid-drag

    QMetaObject::invokeMethod(this, "doDrag", Qt::QueuedConnection, Q_ARG(QQuickItem *, item), Q_ARG(QUrl, url), Q_ARG(QPixmap, pixmap));
}

void DragHelper::doDrag(QQuickItem *item, const QUrl &url, const QPixmap &pixmap)
{
    if (item && item->window() && item->window()->mouseGrabberItem()) {
        item->window()->mouseGrabberItem()->ungrabMouse();
    }

    QDrag *drag = new QDrag(item);

    QMimeData *mimeData = new QMimeData();

    if (!url.isEmpty()) {
        mimeData->setUrls(QList<QUrl>() << url);
    }

    drag->setMimeData(mimeData);

    if (!pixmap.isNull()) {
        drag->setPixmap(pixmap);
    }

    m_dragActive = true;
    Q_EMIT dragActiveChanged();

    drag->exec();

    m_dragActive = false;
    Q_EMIT dragActiveChanged();
}
