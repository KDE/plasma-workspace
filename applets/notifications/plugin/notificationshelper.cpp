/*
    Copyright (C) 2014  Martin Klapetek <mklapetek@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "notificationshelper.h"

#include <QGuiApplication>
#include <qfontmetrics.h>
#include <QTimer>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QReadWriteLock>
#include <QDebug>

NotificationsHelper::NotificationsHelper(QObject *parent)
    : QObject(parent),
    m_popupLocation(NotificationsHelper::BottomRight),
    m_busy(false)
{
    m_mutex = new QReadWriteLock(QReadWriteLock::Recursive);
    m_offset = QFontMetrics(QGuiApplication::font()).boundingRect(QStringLiteral("M")).height();

    m_dispatchTimer = new QTimer(this);
    m_dispatchTimer->setInterval(500);
    m_dispatchTimer->setSingleShot(true);
    connect(m_dispatchTimer, &QTimer::timeout, [this](){m_busy = false; processQueues();});
}

NotificationsHelper::~NotificationsHelper()
{
    qDeleteAll(m_availablePopups);
    qDeleteAll(m_popupsOnScreen);
    delete m_mutex;
}

void NotificationsHelper::setPopupLocation(PositionOnScreen popupLocation)
{
    if (m_popupLocation != popupLocation) {
        m_popupLocation = popupLocation;
        emit popupLocationChanged();

        repositionPopups();
    }
}

void NotificationsHelper::setPlasmoidScreenGeometry(const QRect &plasmoidScreenGeometry)
{
    m_plasmoidScreen = plasmoidScreenGeometry;
    repositionPopups();
}

void NotificationsHelper::addNotificationPopup(QObject *win)
{
    QQuickWindow *popup = qobject_cast<QQuickWindow*>(win);
    m_availablePopups.append(popup);

    // Don't let QML ever delete this component
    QQmlEngine::setObjectOwnership(win, QQmlEngine::CppOwnership);

    connect(win, SIGNAL(notificationTimeout()),
            this, SLOT(onPopupClosed()));

    connect(popup, &QWindow::heightChanged, this, &NotificationsHelper::repositionPopups, Qt::UniqueConnection);
    connect(popup, &QWindow::visibleChanged, this, &NotificationsHelper::onPopupShown, Qt::UniqueConnection);

    popup->setProperty("initialPositionSet", false);
}

void NotificationsHelper::onPopupShown()
{
    QWindow *popup = qobject_cast<QWindow*>(sender());
    if (!popup || !popup->isVisible()) {
        return;
    }

    // Make sure Dialog lays everything out and gets proper geometry
    QMetaObject::invokeMethod(popup, "updateVisibility", Qt::DirectConnection, Q_ARG(bool, true));

    // Now we can position the popups properly as the geometry is now known
    repositionPopups();
}

void NotificationsHelper::processQueues()
{
    if (m_busy) {
        return;
    }

    m_mutex->lockForRead();
    bool shouldProcessShow = !m_showQueue.isEmpty() && !m_availablePopups.isEmpty();
    m_mutex->unlock();

    if (shouldProcessShow) {
        m_busy = true;
        processShow();
        // Return here, makes the movement more clear and easier to follow
        return;
    }

    m_mutex->lockForRead();
    bool shouldProcessHide = !m_hideQueue.isEmpty();
    m_mutex->unlock();

    if (shouldProcessHide) {
        m_busy = true;
        processHide();
    }
}

void NotificationsHelper::processShow()
{
    m_mutex->lockForWrite();
    const QVariantMap notificationData = m_showQueue.takeFirst();
    m_mutex->unlock();

    QString sourceName = notificationData.value(QStringLiteral("source")).toString();

    // Try getting existing popup for the given source
    // (case of notification being just updated)
    QQuickWindow *popup = m_sourceMap.value(sourceName);

    if (!popup) {
        // No existing notification for the given source,
        // take one from the available popups
        m_mutex->lockForWrite();
        popup = m_availablePopups.takeFirst();
        m_popupsOnScreen << popup;
        m_sourceMap.insert(sourceName, popup);
        m_mutex->unlock();
        // Set the source name directly on the popup object too
        // to avoid looking up the notificationProperties map as above
        popup->setProperty("sourceName", sourceName);
    }

    // Populate the popup with data, this is the component's own QML method
    QMetaObject::invokeMethod(popup, "populatePopup", Qt::DirectConnection, Q_ARG(QVariant, notificationData));

    QTimer::singleShot(300, popup, &QWindow::show);

    if (!m_dispatchTimer->isActive()) {
        m_dispatchTimer->start();
    }
}

void NotificationsHelper::processHide()
{
    m_mutex->lockForWrite();
    QQuickWindow *popup = m_hideQueue.takeFirst();
    m_mutex->unlock();

    if (popup) {
        m_mutex->lockForWrite();
        // Remove the popup from the active list and return it into the available list
        m_popupsOnScreen.removeAll(popup);
        m_sourceMap.remove(popup->property("sourceName").toString());
        if (!m_availablePopups.contains(popup)) {
            // make extra sure that pointers in here aren't doubled
            m_availablePopups.append(popup);
        }
        m_mutex->unlock();

        popup->hide();

        // Make sure the popup gets placed correctly
        // next time it's put on screen
        popup->setProperty("initialPositionSet", false);
    }

    m_mutex->lockForRead();
    bool shouldReposition = !m_popupsOnScreen.isEmpty();// && m_showQueue.isEmpty();
    m_mutex->unlock();

    if (shouldReposition) {
        repositionPopups();
    }

    if (!m_dispatchTimer->isActive()) {
        m_dispatchTimer->start();
    }
}

void NotificationsHelper::displayNotification(const QVariantMap &notificationData)
{
    if (notificationData.isEmpty()) {
        return;
    }

    QVariant sourceName = notificationData.value(QStringLiteral("source"));

    // first check if we don't already have data for the same source
    // which would mean that the notification was just updated
    // so remove the old one and append the newest data only
    QMutableListIterator<QVariantMap> i(m_showQueue);
    while (i.hasNext()) {
        if (i.next().value(QStringLiteral("source")) == sourceName) {
            m_mutex->lockForWrite();
            i.remove();
            m_mutex->unlock();
        }
    }

    // ...also look into the hide queue, if it's already queued
    // for hiding, we need to remove it from there otherwise
    // it will get closed too soon
    QMutableListIterator<QQuickWindow*> j(m_hideQueue);
    while (j.hasNext()) {
        if (j.next()->property("sourceName") == sourceName) {
            m_mutex->lockForWrite();
            j.remove();
            m_mutex->unlock();
        }
    }

    m_mutex->lockForWrite();
    m_showQueue.append(notificationData);
    m_mutex->unlock();

    if (!m_dispatchTimer->isActive()) {
        // If the dispatch timer is not already running, process
        // the queues directly, that should cut the time between
        // notification emitting the event and popup displaying
        processQueues();
    }
}

void NotificationsHelper::closePopup(const QString &sourceName)
{
    QQuickWindow *popup = m_sourceMap.value(sourceName);

    m_mutex->lockForRead();
    bool shouldQueue = popup && !m_hideQueue.contains(popup);
    m_mutex->unlock();

    // Make sure the notification that was closed (programatically)
    // is not in the show queue. This is important otherwise that
    // notification will be shown and then never closed (because
    // the close event arrives here, before it's even shown)
    QMutableListIterator<QVariantMap> i(m_showQueue);
    while (i.hasNext()) {
        if (i.next().value(QStringLiteral("source")) == sourceName) {
            m_mutex->lockForWrite();
            i.remove();
            m_mutex->unlock();
        }
    }

    if (shouldQueue) {
        m_mutex->lockForWrite();
        m_hideQueue.append(popup);
        m_mutex->unlock();

        if (!m_dispatchTimer->isActive()) {
            processQueues();
        }
    }
}

void NotificationsHelper::onPopupClosed()
{
    QQuickWindow *popup = qobject_cast<QQuickWindow*>(sender());

    m_mutex->lockForRead();
    bool shouldQueue = popup && !m_hideQueue.contains(popup);
    m_mutex->unlock();

    if (shouldQueue) {
        m_mutex->lockForWrite();
        m_hideQueue << popup;
        m_mutex->unlock();

        if (!m_dispatchTimer->isActive()) {
            processQueues();
        }
    }
}

void NotificationsHelper::repositionPopups()
{
    int cumulativeHeight = m_offset;

    m_mutex->lockForWrite();

    for (int i = 0; i < m_popupsOnScreen.size(); ++i) {
        if (m_popupLocation == NotificationsHelper::TopLeft
            || m_popupLocation == NotificationsHelper::TopCenter
            || m_popupLocation == NotificationsHelper::TopRight) {

            int posY = m_plasmoidScreen.top() + cumulativeHeight;

            if (m_popupsOnScreen[i]->isVisible() && m_popupsOnScreen[i]->property("initialPositionSet").toBool() == true && m_popupsOnScreen[i]->y() != 0) {
                //if it's visible, go through setProperty which animates it
                m_popupsOnScreen[i]->setProperty("y", posY);
            } else {
                // ...otherwise just set it directly
                m_popupsOnScreen[i]->setY(posY);
                m_popupsOnScreen[i]->setProperty("initialPositionSet", true);
            }
        } else {
            int posY = m_plasmoidScreen.bottom() - cumulativeHeight - m_popupsOnScreen[i]->contentItem()->height();

            if (m_popupsOnScreen[i]->isVisible() && m_popupsOnScreen[i]->property("initialPositionSet").toBool() == true && m_popupsOnScreen[i]->y() != 0) {
                m_popupsOnScreen[i]->setProperty("y", posY);
            } else {
                m_popupsOnScreen[i]->setY(posY);
                m_popupsOnScreen[i]->setProperty("initialPositionSet", true);
            }
        }

        switch (m_popupLocation) {
            case Default:
                //This should not happen as the defualt handling is in NotificationApplet::onScreenPositionChanged
                Q_ASSERT(false);
                qWarning("Notication popupLocation is still \"default\". This should not happen");
                //fall through to top right
            case TopRight:
            case BottomRight:
                m_popupsOnScreen[i]->setX(m_plasmoidScreen.right() - m_popupsOnScreen[i]->contentItem()->width() - m_offset);
                break;
            case TopCenter:
            case BottomCenter:
                m_popupsOnScreen[i]->setX(m_plasmoidScreen.x() + (m_plasmoidScreen.width() / 2) - (m_popupsOnScreen[i]->contentItem()->width() / 2));
                break;
            case TopLeft:
            case BottomLeft:
                m_popupsOnScreen[i]->setX(m_plasmoidScreen.left() + m_offset);
                break;
            case Left:
            case Center:
            case Right:
                // Fall-through to make the compiler happy
                break;
        }

        cumulativeHeight += (m_popupsOnScreen[i]->contentItem()->height() + m_offset);
    }

    m_mutex->unlock();
}


