/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dashedbackground.h"

#include <QPainter>
#include <QPainterPath>

DashedBackground::DashedBackground(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

QColor DashedBackground::color() const
{
    return m_color;
}

void DashedBackground::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        update();
        Q_EMIT colorChanged();
    }
}

void DashedBackground::paint(QPainter *painter)
{
    const QRectF bounds = boundingRect();

    const qreal majorAxis = std::sqrt(bounds.width() * bounds.width() + bounds.height() * bounds.height());
    const qreal minorAxis = std::max(bounds.width(), bounds.height());

    const qreal thickness = 20;
    const qreal spacing = 20;

    QColor color = m_color;
    color.setAlphaF(0.1);

    QPainterPath clipPath;
    clipPath.addRoundedRect(boundingRect(), 5, 5);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setClipPath(clipPath);
    painter->translate(bounds.width() / 2, bounds.height() / 2);
    painter->rotate(-60);
    for (qreal offset = -majorAxis / 2; offset < majorAxis / 2; offset += thickness + spacing) {
        painter->fillRect(QRectF(-minorAxis / 2, offset, minorAxis, thickness), color);
    }
}

#include "moc_dashedbackground.cpp"
