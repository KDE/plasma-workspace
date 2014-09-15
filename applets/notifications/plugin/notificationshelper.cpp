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
#include <QQmlEngine>
#include <QDebug>

NotificationsHelper::NotificationsHelper(QObject *parent)
    : QObject(parent),
    m_popupLocation(Qt::BottomEdge)
{
    m_offset = QFontMetrics(QGuiApplication::font()).boundingRect("M").height();
}

NotificationsHelper::~NotificationsHelper()
{
    qDeleteAll(m_availablePopups);
    qDeleteAll(m_popupsOnScreen);
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

    connect(popup, SIGNAL(visibleChanged(bool)),
            this, SLOT(popupClosed(bool)));
}

void NotificationsHelper::displayQueuedNotification()
{
    if (!m_queue.isEmpty() && !m_availablePopups.isEmpty()) {
        displayNotification(m_queue.takeFirst());
    }
}

void NotificationsHelper::displayNotification(const QVariantMap &notificationData)
{
    if (notificationData.isEmpty()) {
        return;
    }

    QString sourceName = notificationData.value("source").toString();

    // All our popups are full, so put it into queue and bail out
    if (m_availablePopups.isEmpty()) {
        // ...but first check if we don't already have data for the same source
        // which would mean that the notification was just updated
        // so remove the old one and append the newest data only
        Q_FOREACH (const QVariantMap &data, m_queue) {
            if (data.value("source").toString() == sourceName) {
                m_queue.removeOne(data);
            }
        }
        m_queue.append(notificationData);
        return;
    }

    // Try getting existing popup for the given source
    // (case of notification being just updated)
    QQuickWindow *popup = m_sourceMap.value(sourceName);

    if (!popup) {
        // No existing notification for the given source,
        // take one from the available popups
        popup = m_availablePopups.takeFirst();
        m_popupsOnScreen << popup;
        m_sourceMap.insert(sourceName, popup);
        // Set the source name directly on the popup object too
        // to avoid looking up the notificationProperties map as above
        popup->setProperty("sourceName", sourceName);

        // Set the height to random small number so that we actually receive
        // the signal connected below
        popup->setHeight(1);
    }

    // Populate the popup with data, this is the component's own QML method
    QMetaObject::invokeMethod(popup, "populatePopup", Q_ARG(QVariant, notificationData));

    QMetaObject::Connection popupHeightConnection;

    // Here we need to wait for the notification popup's Dialog to actually
    // sync its size to the mainItem size, only then we can move on with
    // positioning it properly
    popupHeightConnection = connect(popup, &QWindow::heightChanged, [=](){
        QRect screenArea = workAreaForScreen(m_plasmoidScreen);

        popup->setX(screenArea.right() - popup->width() - m_offset);

        int popupsHeightTotal = 0;
        // notification popups with 3 buttons are higher than the rest
        // so we need to count the heights separately
        Q_FOREACH (QQuickWindow *popupOnScreen, m_popupsOnScreen) {
            popupsHeightTotal += popupOnScreen->height() + m_offset;
        }

        if (m_popupLocation == Qt::TopEdge) {
            popup->setY(screenArea.top() + popupsHeightTotal - popup->height());
        } else {
            popup->setY(screenArea.bottom() - popupsHeightTotal);
        }

        popup->show();

        // don't keep the connection around
        QObject::disconnect(popupHeightConnection);
    });
}

void NotificationsHelper::closePopup(const QString &sourceName)
{
    QQuickWindow *popup = m_sourceMap.value(sourceName);

    if (popup) {
        popup->hide();
    }
}

void NotificationsHelper::popupClosed(bool visible)
{
    if (!visible) {
        QQuickWindow *popup = qobject_cast<QQuickWindow*>(sender());
        if (popup) {
            // Remove the popup from the active list and return it into the available list
            m_popupsOnScreen.removeOne(popup);
            m_sourceMap.remove(sender()->property("sourceName").toString());
            m_availablePopups.append(popup);
        }
        repositionPopups();
    }

}

void NotificationsHelper::repositionPopups()
{
    QRect workArea = workAreaForScreen(m_plasmoidScreen);

    int cumulativeHeight = m_offset;

    for (int i = 0; i < m_popupsOnScreen.size(); ++i) {

        if (m_popupLocation == Qt::TopEdge) {
            m_popupsOnScreen[i]->setProperty("y", workArea.top() + cumulativeHeight);
        } else {
            m_popupsOnScreen[i]->setProperty("y", workArea.bottom() - cumulativeHeight - m_popupsOnScreen[i]->height());
        }

        cumulativeHeight += (m_popupsOnScreen[i]->height() + m_offset);
    }

    if (!m_queue.isEmpty()) {
        // Delay this a bit so the animations above^ have time to finish; looks a lot better
        QTimer::singleShot(300, this, SLOT(displayQueuedNotification()));
    }

}

#include "notificationshelper.moc"
