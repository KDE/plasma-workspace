/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QPointer>
#include <QQuickItem>

class WheelInterceptor : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *destination READ destination WRITE setDestination NOTIFY destinationChanged)

public:
    explicit WheelInterceptor(QQuickItem *parent = nullptr);
    ~WheelInterceptor() override;

    QQuickItem *destination() const;
    void setDestination(QQuickItem *destination);

    Q_INVOKABLE QQuickItem *findWheelArea(QQuickItem *parent) const;

Q_SIGNALS:
    void destinationChanged() const;
    void wheelMoved(QPoint delta) const;

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    QPointer<QQuickItem> m_destination;
};
