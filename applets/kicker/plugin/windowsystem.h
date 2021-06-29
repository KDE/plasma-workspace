/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QQuickWindow>
class QQuickItem;

class WindowSystem : public QObject
{
    Q_OBJECT

public:
    explicit WindowSystem(QObject *parent = nullptr);
    ~WindowSystem() override;

    bool eventFilter(QObject *watched, QEvent *event) override;

    Q_INVOKABLE void forceActive(QQuickItem *item);

    Q_INVOKABLE bool isActive(QQuickItem *item);

    Q_INVOKABLE void monitorWindowFocus(QQuickItem *item);

    Q_INVOKABLE void monitorWindowVisibility(QQuickItem *item);

Q_SIGNALS:
    void focusIn(QQuickWindow *window) const;
    void hidden(QQuickWindow *window) const;

private Q_SLOTS:
    void monitoredWindowVisibilityChanged(QWindow::Visibility visibility) const;
};
