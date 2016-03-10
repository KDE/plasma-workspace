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

#include "../plugin/notificationshelper.h"

class NotificationsApplet : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(uint screenPosition READ screenPosition WRITE onScreenPositionChanged NOTIFY screenPositionChanged)
    Q_PROPERTY(QRect availableScreenRect READ availableScreenRect NOTIFY availableScreenRectChanged)

public:
    NotificationsApplet(QObject *parent, const QVariantList &data);
    ~NotificationsApplet() override;

    uint screenPosition() const;

    // This is the screen position that is stored
    // in the config file, used to initialize the
    // applet settings dialog
    Q_INVOKABLE uint configScreenPosition() const;

    QRect availableScreenRect() const;

public Q_SLOTS:
    void init() Q_DECL_OVERRIDE;
    void onScreenPositionChanged(uint position);
    void onAppletLocationChanged();

Q_SIGNALS:
    void screenPositionChanged(uint position);
    void availableScreenRectChanged(const QRect &availableScreenRect);

private:
    void setScreenPositionFromAppletLocation();
    void onScreenChanges();

    NotificationsHelper::PositionOnScreen m_popupPosition;
    QRect m_availableScreenRect;
};


#endif // NOTIFICATIONS_APPLET
