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

#ifndef NOTIFICATIONSHELPER_H
#define NOTIFICATIONSHELPER_H

#include <QObject>
#include <QRect>
#include <QHash>
#include <QVariantMap>

class QQuickWindow;
class QTimer;
class QReadWriteLock;

class NotificationsHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PositionOnScreen popupLocation MEMBER m_popupLocation WRITE setPopupLocation NOTIFY popupLocationChanged)

public:
    enum PositionOnScreen {
        Default, // Follows the panel
        TopLeft,
        TopCenter,
        TopRight,
        Left,
        Center,
        Right,
        BottomLeft,
        BottomCenter,
        BottomRight
    };
    Q_ENUM(PositionOnScreen)

    NotificationsHelper(QObject *parent = 0);
    ~NotificationsHelper() override;
    Q_INVOKABLE void addNotificationPopup(QObject *win);
    Q_INVOKABLE void closePopup(const QString &sourceName);

    Q_INVOKABLE void setPlasmoidScreenGeometry(const QRect &geometry);

    void setPopupLocation(PositionOnScreen popupLocation);

    /**
     * Fills the popup with data from notificationData
     * and puts the popup on proper place on screen.
     * If there's no space on screen for the notification,
     * it's queued and displayed as soon as there's space for it
     */
    Q_INVOKABLE void displayNotification(const QVariantMap &notificationData);

Q_SIGNALS:
    void popupLocationChanged();
//     void plasmoidScreenChanged();

private Q_SLOTS:
    void onPopupShown();
    void onPopupClosed();
    void processQueues();
    void processShow();
    void processHide();

private:
    void repositionPopups();

    QList<QQuickWindow*> m_popupsOnScreen;
    QList<QQuickWindow*> m_availablePopups;
    QHash<QString, QQuickWindow*> m_sourceMap;
    QRect m_plasmoidScreen;
    PositionOnScreen m_popupLocation;
    int m_offset;
    bool m_busy;
    QList<QQuickWindow*> m_hideQueue;
    QList<QVariantMap> m_showQueue;
    QReadWriteLock *m_mutex;
    QTimer *m_dispatchTimer;
};

#endif // NOTIFICATIONSHELPER_H
