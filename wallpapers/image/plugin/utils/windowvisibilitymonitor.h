/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QPointer>
#include <QQuickWindow>
#include <QtQml/qqmlregistration.h>

/**
 * This class monitors whether the given window is actually visible on screen.
 *
 * On Wayland the compositor stops sending frame callbacks when the output is
 * turned off, and Qt marks the window as not exposed after a short timeout.
 * This lets us pause an animated wallpaper when the monitor is turned off
 * (e.g. by DPMS) or when the window is otherwise not visible.
 */
class WindowVisibilityMonitor : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool exposed READ exposed NOTIFY exposedChanged)
    Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)

public:
    explicit WindowVisibilityMonitor(QObject *parent = nullptr);
    ~WindowVisibilityMonitor() override;

    bool exposed() const;
    QQuickWindow *window() const;
    void setWindow(QQuickWindow *window);

Q_SIGNALS:
    void exposedChanged();
    void windowChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateExposed();

    bool m_exposed = false;
    QPointer<QQuickWindow> m_window;
};
