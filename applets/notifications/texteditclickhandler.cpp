/*
    Copyright (C) 2021 Kai Uwe Broulik <kde@broulik.de>

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

#include "texteditclickhandler.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QQuickItem>
#include <QStyleHints>

TextEditClickHandler::TextEditClickHandler(QObject *parent)
    : QObject(parent)
{
}

TextEditClickHandler::~TextEditClickHandler() = default;

QQuickItem *TextEditClickHandler::target() const
{
    return m_target.data();
}

void TextEditClickHandler::setTarget(QQuickItem *target)
{
    if (m_target.data() == target) {
        return;
    }

    if (m_target) {
        m_target->removeEventFilter(this);
    }

    m_target = target;
    m_target->installEventFilter(this);
    Q_EMIT targetChanged(target);
}

bool TextEditClickHandler::eventFilter(QObject *watched, QEvent *event)
{
    Q_ASSERT(watched == m_target.data());

    if (event->type() == QEvent::MouseButtonPress) {
        const auto *e = static_cast<QMouseEvent *>(event);
        m_pressPos = e->pos();
    } else if (event->type() == QEvent::MouseButtonRelease) {
        const auto *e = static_cast<QMouseEvent *>(event);

        if (m_pressPos.x() > -1 && m_pressPos.y() > -1
                && (m_pressPos - e->pos()).manhattanLength() < qGuiApp->styleHints()->startDragDistance()) {
            Q_EMIT clicked();
        }
    }

    return false;
}
