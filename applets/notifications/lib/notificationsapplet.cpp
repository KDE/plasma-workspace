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

#include "notificationsapplet.h"

#include <QDebug>

#include <KConfigGroup>

NotificationsApplet::NotificationsApplet(QObject *parent, const QVariantList &data)
    : Plasma::Applet(parent, data)
{
    connect(this, &Plasma::Applet::locationChanged,
            this, &NotificationsApplet::onAppletLocationChanged);
}

NotificationsApplet::~NotificationsApplet()
{
}

void NotificationsApplet::init()
{
    KConfigGroup globalGroup = globalConfig();
    m_popupPosition = (NotificationsHelper::PositionOnScreen)globalGroup.readEntry("popupPosition", 0); //0 is default

    Plasma::Applet::init();
}

void NotificationsApplet::onAppletLocationChanged(Plasma::Types::Location location)
{
    if (globalConfig().readEntry("popupPosition", 0) == 0) {
        // If the screenPosition is the default, follow the panel
        if (location == Plasma::Types::TopEdge) {
            if (QGuiApplication::isRightToLeft()) {
                m_popupPosition = NotificationsHelper::TopLeft;
            } else {
                m_popupPosition = NotificationsHelper::TopRight;
            }
        } else {
            if (QGuiApplication::isRightToLeft()) {
                m_popupPosition = NotificationsHelper::BottomLeft;
            } else {
                m_popupPosition = NotificationsHelper::BottomRight;
            }
        }

        Q_EMIT screenPositionChanged(m_popupPosition);
    }
}

uint NotificationsApplet::screenPosition() const
{
    return m_popupPosition;
}

void NotificationsApplet::onScreenPositionChanged(uint position)
{
    KConfigGroup globalGroup = globalConfig();
    globalGroup.writeEntry("popupPosition", position);
    m_popupPosition = (NotificationsHelper::PositionOnScreen)position;

    Q_EMIT screenPositionChanged(position);
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(notifications, NotificationsApplet, "metadata.json")

#include "notificationsapplet.moc"
