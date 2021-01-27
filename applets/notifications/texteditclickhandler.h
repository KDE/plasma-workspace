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

#pragma once

#include <QObject>
#include <QPointer>
#include <QPointF>

class QQuickItem;

/**
 * Simple event filter that emits a clicked() signal when clicking
 * on a TextEdit while still letting the user select text like normal.
 *
 * It's just for this very specific use case, which is also why it has
 * no acceptedButtons, MouseEvent argument on clicked signal, etc.
 */
class TextEditClickHandler : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)

public:
    explicit TextEditClickHandler(QObject *parent = nullptr);
    ~TextEditClickHandler() override;

    QQuickItem *target() const;
    void setTarget(QQuickItem *target);
    Q_SIGNAL void targetChanged(QQuickItem *target);

    bool eventFilter(QObject *watched, QEvent *event) override;

Q_SIGNALS:
    void clicked();

private:
    QPointer<QQuickItem> m_target;
    QPointF m_pressPos{-1, -1};

};
