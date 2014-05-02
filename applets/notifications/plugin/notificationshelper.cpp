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
    : QObject(parent)
{
    m_offset = QFontMetrics(QGuiApplication::font()).boundingRect("M").height() * 2;
}

NotificationsHelper::~NotificationsHelper()
{
    qDeleteAll(m_availablePopups);
    qDeleteAll(m_popupsOnScreen);
}

QRect NotificationsHelper::workAreaForScreen(int screenId)
{
    QRect workArea = KWindowSystem::workArea();

    return qApp->screens().at(screenId)->availableGeometry().intersected(workArea);
}

void NotificationsHelper::setPlasmoidScreen(int screenId)
{
    m_plasmoidScreen = screenId;
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

    // All our popups are full, so put it into queue and bail out
    if (m_availablePopups.isEmpty()) {
        m_queue.append(notificationData);
        return;
    }

    QQuickWindow *popup = m_availablePopups.takeFirst();
    m_popupsOnScreen << popup;

    QString sourceName = notificationData.value("source").toString();

    m_sourceMap.insert(sourceName, popup);

    // Set the source name directly on the popup object too
    // to avoid looking up the notificationProperties map as above
    popup->setProperty("sourceName", sourceName);

    QRect screenArea = workAreaForScreen(m_plasmoidScreen);

    popup->setX(screenArea.x() + screenArea.width() - popup->width() - m_offset);
    popup->setY(screenArea.height() - (m_popupsOnScreen.size() * (popup->height() + m_offset)));

    // Populate the popup with data, this is the component's own QML method
    QMetaObject::invokeMethod(popup, "populatePopup", Q_ARG(QVariant, notificationData));

    popup->show();
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
    for (int i = 0; i < m_popupsOnScreen.size(); i++) {
        m_popupsOnScreen[i]->setProperty("y", workAreaForScreen(m_plasmoidScreen).height() - ((i + 1) * (m_popupsOnScreen[i]->height() + m_offset)));
    }

    if (!m_queue.isEmpty()) {
        // Delay this a bit so the animations above^ have time to finish; looks a lot better
        QTimer::singleShot(300, this, SLOT(displayQueuedNotification()));
    }

}

#include "notificationshelper.moc"
