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

#include <KConfigGroup>
#include <KWindowSystem>

#include <Plasma/Containment>
#include <Plasma/Corona>

#include <QDebug>

NotificationsApplet::NotificationsApplet(QObject *parent, const QVariantList &data)
    : Plasma::Applet(parent, data),
      m_availableScreenRect(0,0,0,0)
{
}

NotificationsApplet::~NotificationsApplet()
{
}

void NotificationsApplet::init()
{
    m_popupPosition = (NotificationsHelper::PositionOnScreen)configScreenPosition();

    connect(this, &Plasma::Applet::locationChanged,
            this, &NotificationsApplet::onAppletLocationChanged);

    connect(containment(), &Plasma::Containment::screenChanged,
            this, &NotificationsApplet::onScreenChanges);

    Q_ASSERT(containment());
    Q_ASSERT(containment()->corona());
    connect(containment()->corona(), &Plasma::Corona::availableScreenRectChanged, this, &NotificationsApplet::onScreenChanges);

    Plasma::Applet::init();

    onScreenChanges();
    onAppletLocationChanged();
}

void NotificationsApplet::onScreenChanges()
{
    // when removing the panel the applet is in, the containment is being destroyed but its corona is still
    // there, rightfully emitting availableScreenRectChanged and then we blow up if we try to access it.
    if (!containment() || !containment()->corona()) {
        return;
    }

    auto newAvailableScreenRect = containment()->corona()->availableScreenRect(containment()->screen());
    if (newAvailableScreenRect != m_availableScreenRect) {
        m_availableScreenRect = newAvailableScreenRect;
        Q_EMIT availableScreenRectChanged(m_availableScreenRect);
    }
}

QRect NotificationsApplet::availableScreenRect() const
{
    return m_availableScreenRect;
}

void NotificationsApplet::onAppletLocationChanged()
{
    if (configScreenPosition() == 0) {
        // If the screenPosition is set to default,
        // just follow the panel
        setScreenPositionFromAppletLocation();
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
    globalGroup.sync();

    // If the position is set to default, let the setScreenPositionFromAppletLocation()
    // figure out the effective position, otherwise just set it to m_popupPosition
    // and emit the change
    if (position == NotificationsHelper::Default) {
        setScreenPositionFromAppletLocation();
    } else if (m_popupPosition != position) {
        m_popupPosition = (NotificationsHelper::PositionOnScreen)position;
        Q_EMIT screenPositionChanged(m_popupPosition);
    }
}

uint NotificationsApplet::configScreenPosition() const
{
    KConfigGroup globalGroup = globalConfig();
    return globalGroup.readEntry("popupPosition", 0); //0 is default
}

void NotificationsApplet::setScreenPositionFromAppletLocation()
{
    NotificationsHelper::PositionOnScreen newPopupPosition = m_popupPosition;
    if (location() == Plasma::Types::TopEdge) {
        if (QGuiApplication::isRightToLeft()) {
            newPopupPosition = NotificationsHelper::TopLeft;
        } else {
            newPopupPosition = NotificationsHelper::TopRight;
        }
    } else {
        if (QGuiApplication::isRightToLeft()) {
            newPopupPosition = NotificationsHelper::BottomLeft;
        } else {
            newPopupPosition = NotificationsHelper::BottomRight;
        }
    }

    if (newPopupPosition != m_popupPosition) {
        m_popupPosition = newPopupPosition;
        Q_EMIT screenPositionChanged(m_popupPosition);
    }
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(notifications, NotificationsApplet, "metadata.json")

#include "notificationsapplet.moc"
