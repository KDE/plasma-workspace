/*
 * Copyright 2014 (c) Martin Klapetek <mklapetek@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef NOTIFICATIONS_APPLET_H
#define NOTIFICATIONS_APPLET_H

#include <Plasma/Applet>

class NotificationsApplet : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(uint screenPosition READ screenPosition WRITE onScreenPositionChanged NOTIFY screenPositionChanged)

public:
    NotificationsApplet(QObject *parent, const QVariantList &data);
    ~NotificationsApplet();

    Q_INVOKABLE uint screenPosition() const;

public Q_SLOTS:
    void onScreenPositionChanged(uint position);

Q_SIGNALS:
    void screenPositionChanged(uint position);

private:

};


#endif // NOTIFICATIONS_APPLET
