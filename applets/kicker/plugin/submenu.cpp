/*
    SPDX-FileCopyrightText: 2014 David Edmundson <kde@davidedmundson.co.uk>
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "submenu.h"

#include <cmath>

#include <QScreen>

#include <KWindowSystem>

SubMenu::SubMenu(QQuickItem *parent)
    : PlasmaQuick::Dialog(parent)
    , m_offset(0)
    , m_facingLeft(false)
{
    setType(AppletPopup);
}

SubMenu::~SubMenu()
{
}

int SubMenu::offset() const
{
    return m_offset;
}

void SubMenu::setOffset(int offset)
{
    if (m_offset != offset) {
        m_offset = offset;

        Q_EMIT offsetChanged();
    }
}

QPoint SubMenu::popupPosition(QQuickItem *item, const QSize &size)
{
    if (!item || !item->window()) {
        return QPoint(0, 0);
    }

    QPointF pos = item->mapToScene(QPointF(0, 0));
    pos = item->window()->mapToGlobal(pos.toPoint());

    pos.setX(pos.x() + m_offset + item->width());

    QRect avail = availableScreenRectForItem(item);

    if (pos.x() + size.width() > avail.right()) {
        pos.setX(pos.x() - m_offset - item->width() - size.width());

        m_facingLeft = true;
        Q_EMIT facingLeftChanged();
    }

    pos.setY(pos.y() - margins()->property("top").toInt());

    if (pos.y() + size.height() > avail.bottom()) {
        int overshoot = std::ceil(((avail.bottom() - (pos.y() + size.height())) * -1) / item->height()) * item->height();

        pos.setY(pos.y() - overshoot);
    }

    return pos.toPoint();
}

QRect SubMenu::availableScreenRectForItem(QQuickItem *item) const
{
    QScreen *screen = QGuiApplication::primaryScreen();

    const QPoint globalPosition = item->window()->mapToGlobal(item->position().toPoint());

    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *s : screens) {
        if (s->geometry().contains(globalPosition)) {
            screen = s;
        }
    }

    return screen->availableGeometry();
}
