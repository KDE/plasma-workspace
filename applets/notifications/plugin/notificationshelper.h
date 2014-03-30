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

class QQuickWindow;

class NotificationsHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int plasmoidScreen WRITE setPlasmoidScreen)

public:
    Q_INVOKABLE QRect workAreaForScreen(int screenId);
    Q_INVOKABLE void positionPopup(QObject *win);
    Q_INVOKABLE void closePopup(const QString &sourceName);

    void setPlasmoidScreen(int screenId);

private Q_SLOTS:
    void popupClosed(bool visible);

private:
    void repositionPopups();

    QList<QQuickWindow*> m_popups;
    QList<QQuickWindow*> m_queuedPopups;
    QHash<QString, QQuickWindow*> m_sourceMap;
    int m_plasmoidScreen;
};

#endif // NOTIFICATIONSHELPER_H
