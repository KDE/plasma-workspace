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
#include <kwindowsystem.h>

#include <QScreen>
#include <QGuiApplication>
#include <qfontmetrics.h>
#include <QTimer>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QTimer>
#include <QReadWriteLock>
#include <QDebug>

NotificationsHelper::NotificationsHelper(QObject *parent)
    : QObject(parent),
    m_popupLocation(Qt::BottomEdge),
    m_busy(false)
{
    m_mutex = new QReadWriteLock(QReadWriteLock::Recursive);
    m_offset = QFontMetrics(QGuiApplication::font()).boundingRect("M").height();

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

void NotificationsHelper::setPopupLocation(Qt::Edge popupLocation)
{
    if (m_popupLocation != popupLocation) {
        m_popupLocation = popupLocation;
        emit popupLocationChanged();

        repositionPopups();
    }
}

QRect NotificationsHelper::workAreaForScreen(const QRect &screenGeometry)
{
    QRect workArea = KWindowSystem::workArea();
    Q_FOREACH (QScreen *screen, qApp->screens()) {
        QRect geo = screen->geometry();
        if (geo.contains(screenGeometry.center())) {
            return geo.intersected(workArea);
        }
    }

    return workArea;
}

void NotificationsHelper::addNotificationPopup(QObject *win)
{
    QQuickWindow *popup = qobject_cast<QQuickWindow*>(win);
    m_availablePopups.append(popup);

    // Don't let QML ever delete this component
    QQmlEngine::setObjectOwnership(win, QQmlEngine::CppOwnership);

    connect(win, SIGNAL(notificationTimeout()),
            this, SLOT(onPopupClosed()));

    // The popup actual height will get changed after it has been filled
    // with data, but we set it initially to small value so it's not too
    // big for eg. one line notifications
    popup->setHeight(1);

    // This is to make sure the popups won't fly across the whole
    // screen the first time they appear
    if (m_popupLocation == Qt::TopEdge) {
        popup->setY(0);
    } else {
        popup->setY(workAreaForScreen(m_plasmoidScreen).height());
    }
    popup->setX(workAreaForScreen(m_plasmoidScreen).width() - m_offset - popup->width());

    connect(popup, &QWindow::heightChanged, this, &NotificationsHelper::repositionPopups, Qt::UniqueConnection);
    connect(popup, &QWindow::widthChanged, this, &NotificationsHelper::repositionPopups, Qt::UniqueConnection);
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

    QString sourceName = notificationData.value("source").toString();

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

        // Set the height to random small number so that we actually receive
        // the signal connected below
        popup->setHeight(1);
    }

    // Populate the popup with data, this is the component's own QML method
    QMetaObject::invokeMethod(popup, "populatePopup", Q_ARG(QVariant, notificationData));
    repositionPopups();
    QTimer::singleShot(300, popup, SLOT(show()));

    m_dispatchTimer->start();
}

void NotificationsHelper::processHide()
{
    m_mutex->lockForWrite();
    QQuickWindow *popup = m_hideQueue.takeFirst();
    m_mutex->unlock();

    if (popup) {
        m_mutex->lockForWrite();
        // Remove the popup from the active list and return it into the available list
        m_popupsOnScreen.removeOne(popup);
        m_sourceMap.remove(popup->property("sourceName").toString());
        m_availablePopups.append(popup);
        m_mutex->unlock();

        popup->hide();
        // Shrink the popup again as notifications with lots of text make the popup
        // huge but setting short text won't make it smaller
        popup->setHeight(1);
        // Make sure it flies in from where it's supposed to
        if (m_popupLocation == Qt::TopEdge) {
            popup->setY(0);
        } else {
            popup->setY(workAreaForScreen(m_plasmoidScreen).height());
        }
    }

    m_mutex->lockForRead();
    bool shouldReposition = !m_popupsOnScreen.isEmpty();// && m_showQueue.isEmpty();
    m_mutex->unlock();

    if (shouldReposition) {
        repositionPopups();
    }

    m_dispatchTimer->start();
}

void NotificationsHelper::displayNotification(const QVariantMap &notificationData)
{
    if (notificationData.isEmpty()) {
        return;
    }

    QVariant sourceName = notificationData.value("source");

    // first check if we don't already have data for the same source
    // which would mean that the notification was just updated
    // so remove the old one and append the newest data only
    QMutableListIterator<QVariantMap> i(m_showQueue);
    while (i.hasNext()) {
        if (i.next().value("source") == sourceName) {
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

    processQueues();
}

void NotificationsHelper::closePopup(const QString &sourceName)
{
    QQuickWindow *popup = m_sourceMap.value(sourceName);

    m_mutex->lockForRead();
    bool shouldQueue = popup && !m_hideQueue.contains(popup);
    m_mutex->unlock();

    if (shouldQueue) {
        m_mutex->lockForWrite();
        m_hideQueue.append(popup);
        m_mutex->unlock();
        processQueues();
    }
}

void NotificationsHelper::onPopupClosed()
{
    QQuickWindow *popup = qobject_cast<QQuickWindow*>(sender());

    m_mutex->lockForRead();
    bool shouldQueue = popup || !m_hideQueue.contains(popup);
    m_mutex->unlock();

    if (shouldQueue) {
        m_mutex->lockForWrite();
        m_hideQueue << popup;
        m_mutex->unlock();
        processQueues();
    }
}

void NotificationsHelper::repositionPopups()
{
    QRect workArea = workAreaForScreen(m_plasmoidScreen);

    int cumulativeHeight = m_offset;

    m_mutex->lockForWrite();

    for (int i = 0; i < m_popupsOnScreen.size(); ++i) {
        if (m_popupLocation == Qt::TopEdge) {
            if (m_popupsOnScreen[i]->isVisible()) {
                //if it's visible, go through setProperty which animates it
                m_popupsOnScreen[i]->setProperty("y", workArea.top() + cumulativeHeight);
            } else {
                // ...otherwise just set it directly
                m_popupsOnScreen[i]->setY(workArea.top() + cumulativeHeight);
            }
        } else {
            if (m_popupsOnScreen[i]->isVisible()) {
                m_popupsOnScreen[i]->setProperty("y", workArea.bottom() - cumulativeHeight - m_popupsOnScreen[i]->contentItem()->height());
            } else {
                m_popupsOnScreen[i]->setY(workArea.bottom() - cumulativeHeight - m_popupsOnScreen[i]->contentItem()->height());
            }
        }

        m_popupsOnScreen[i]->setX(workArea.right() - m_popupsOnScreen[i]->contentItem()->width() - m_offset);

        cumulativeHeight += (m_popupsOnScreen[i]->contentItem()->height() + m_offset);
    }

    m_mutex->unlock();
}


