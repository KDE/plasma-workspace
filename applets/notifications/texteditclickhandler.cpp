/*
    SPDX-FileCopyrightText: 2021 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
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

        if (m_pressPos.x() > -1 && m_pressPos.y() > -1 //
            && (m_pressPos - e->pos()).manhattanLength() < qGuiApp->styleHints()->startDragDistance()) {
            Q_EMIT clicked();
        }
    }

    return false;
}
