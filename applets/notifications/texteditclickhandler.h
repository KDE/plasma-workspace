/*
    SPDX-FileCopyrightText: 2021 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QObject>
#include <QPointF>
#include <QPointer>

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
