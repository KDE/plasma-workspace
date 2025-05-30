/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sunpathchart.h"

#include <QPainter>
#include <QPainterPath>

using namespace std::chrono_literals;

static const int secondsInDay = 86400;

SunPathChart::SunPathChart(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

void SunPathChart::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    relayout();
}

void SunPathChart::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);

    if (isComponentComplete()) {
        relayout();
    }
}

void SunPathChart::paint(QPainter *painter)
{
    const auto [daylightProgress, daylightDuration] = daylightProgressDuration(QDateTime::currentDateTime());

    painter->setRenderHint(QPainter::Antialiasing);
    painter->scale(1, -1);
    painter->translate(width() / 2, -height());

    const bool daylight = daylightProgress > 0 && daylightProgress < daylightDuration;
    const QPen pathPen = QPen(daylight ? m_dayColor : m_nightColor, 1);

    QPainterPath path;
    path.moveTo(-m_span, 0);
    path.cubicTo(QPointF(-m_span, height()), QPointF(m_span, height()), QPointF(m_span, 0));
    painter->strokePath(path, pathPen);

    if (daylight) {
        const QPointF sunPosition = path.pointAtPercent(qreal(daylightProgress) / daylightDuration);

        painter->save();
        painter->setBrush(m_dayColor);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(sunPosition, 5, 5);

        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(m_dayColor, 2));
        painter->translate(sunPosition);
        for (int ray = 0; ray < 8; ++ray) {
            painter->save();
            painter->rotate(ray * 360.0 / 8.0);
            painter->drawLine(QPointF(7, 0), QPointF(9, 0));
            painter->restore();
        }

        painter->restore();
    }

    painter->setBrush(Qt::NoBrush);
    painter->setPen(pathPen);
    painter->drawLine(QLineF(QPointF(-width() / 2, 0), QPointF(width() / 2, 0)));
}

QColor SunPathChart::dayColor() const
{
    return m_dayColor;
}

void SunPathChart::setDayColor(const QColor &color)
{
    if (m_dayColor != color) {
        m_dayColor = color;
        if (isComponentComplete()) {
            update();
        }
        Q_EMIT dayColorChanged();
    }
}

QColor SunPathChart::nightColor() const
{
    return m_nightColor;
}

void SunPathChart::setNightColor(const QColor &color)
{
    if (m_nightColor != color) {
        m_nightColor = color;
        if (isComponentComplete()) {
            update();
        }
        Q_EMIT nightColorChanged();
    }
}

QDateTime SunPathChart::sunriseDateTime() const
{
    return m_sunriseDateTime;
}

void SunPathChart::setSunriseDateTime(const QDateTime &dateTime)
{
    if (m_sunriseDateTime != dateTime) {
        m_sunriseDateTime = dateTime;
        if (isComponentComplete()) {
            relayout();
            update();
        }
        Q_EMIT sunriseDateTimeChanged();
    }
}

QDateTime SunPathChart::sunsetDateTime() const
{
    return m_sunsetDateTime;
}

void SunPathChart::setSunsetDateTime(const QDateTime &dateTime)
{
    if (m_sunsetDateTime != dateTime) {
        m_sunsetDateTime = dateTime;
        if (isComponentComplete()) {
            relayout();
            update();
        }
        Q_EMIT sunsetDateTimeChanged();
    }
}

int SunPathChart::daylightSpan() const
{
    return m_span;
}

void SunPathChart::setDaylightSpan(int span)
{
    if (m_span != span) {
        m_span = span;
        Q_EMIT daylightSpanChanged();
    }
}

std::pair<int, int> SunPathChart::daylightProgressDuration(const QDateTime &dateTime) const
{
    const QTime sunriseTime = m_sunriseDateTime.time();
    const QTime sunsetTime = m_sunsetDateTime.time();

    int daylightDuration;
    int daylightProgress;
    if (sunriseTime < sunsetTime) {
        daylightDuration = sunriseTime.secsTo(sunsetTime);
        daylightProgress = sunriseTime.secsTo(dateTime.time());
    } else {
        daylightDuration = secondsInDay - sunsetTime.secsTo(sunriseTime);
        daylightProgress = std::max(sunriseTime.secsTo(dateTime.time()), 0) + std::max(dateTime.time().secsTo(sunsetTime), 0);
    }

    return std::make_pair(daylightProgress, daylightDuration);
}

void SunPathChart::relayout()
{
    const auto [daylightProgress, daylightDuration] = daylightProgressDuration(QDateTime::currentDateTime());

    const qreal minSpan = width() / 5;
    const qreal maxSpan = width() / 2;
    setDaylightSpan(std::lerp(minSpan, maxSpan, qreal(daylightDuration) / secondsInDay));
}
