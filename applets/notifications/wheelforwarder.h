/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QPointer>

class QQuickItem;

class WheelForwarder : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *toItem MEMBER m_toItem)

public:
    using QObject::QObject;
    Q_DISABLE_COPY_MOVE(WheelForwarder)

    Q_INVOKABLE void interceptWheelEvent(QQuickItem *from);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;

    QQuickItem *m_toItem = nullptr;
};
