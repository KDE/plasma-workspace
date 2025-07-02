/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QQuickPaintedItem>

class DashedBackground : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit DashedBackground(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    QColor color() const;
    void setColor(const QColor &color);

Q_SIGNALS:
    void colorChanged();

private:
    QColor m_color;
};
