/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QQuickPaintedItem>

class SunPathChart : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QDateTime sunriseDateTime READ sunriseDateTime WRITE setSunriseDateTime NOTIFY sunriseDateTimeChanged)
    Q_PROPERTY(QDateTime sunsetDateTime READ sunsetDateTime WRITE setSunsetDateTime NOTIFY sunsetDateTimeChanged)
    Q_PROPERTY(QColor dayColor READ dayColor WRITE setDayColor NOTIFY dayColorChanged)
    Q_PROPERTY(QColor nightColor READ nightColor WRITE setNightColor NOTIFY nightColorChanged)
    Q_PROPERTY(int daylightSpan READ daylightSpan NOTIFY daylightSpanChanged)

public:
    explicit SunPathChart(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    QDateTime sunriseDateTime() const;
    void setSunriseDateTime(const QDateTime &dateTime);

    QDateTime sunsetDateTime() const;
    void setSunsetDateTime(const QDateTime &dateTime);

    QColor dayColor() const;
    void setDayColor(const QColor &color);

    QColor nightColor() const;
    void setNightColor(const QColor &color);

    int daylightSpan() const;

Q_SIGNALS:
    void dayColorChanged();
    void nightColorChanged();
    void sunriseDateTimeChanged();
    void sunsetDateTimeChanged();
    void daylightSpanChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void setDaylightSpan(int span);

    std::pair<int, int> daylightProgressDuration(const QDateTime &dateTime) const;
    void relayout();

    QDateTime m_sunriseDateTime;
    QDateTime m_sunsetDateTime;
    QColor m_dayColor;
    QColor m_nightColor;
    int m_span = 0;
};
