/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QColor>
#include <QObject>
#include <QSharedPointer>

class QQuickItem;
class QQuickItemGrabResult;

class EdgeColorSampler : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QColor color READ color NOTIFY colorChanged)

public:
    explicit EdgeColorSampler(QObject *parent = nullptr);

    QQuickItem *source() const;
    void setSource(QQuickItem *source);

    QColor color() const;

Q_SIGNALS:
    void colorChanged();
    void sourceChanged();

    void grabFailed();

private Q_SLOTS:
    void extractColor();

private:
    void update();

    QQuickItem *m_sourceItem = nullptr;
    QSharedPointer<QQuickItemGrabResult> m_grabResult;
    QColor m_color;

    int startX = 0;
    int startY = 0;
    int endX = 0;
    int endY = 0;
};
