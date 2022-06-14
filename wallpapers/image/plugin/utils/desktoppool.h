/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <shared_mutex>
#include <unordered_map>

#include <QObject>
#include <QRect>
#include <QTimer>
#include <QUrl>

class QQuickWindow;

class DesktopPool : public QObject
{
    Q_OBJECT

public:
    explicit DesktopPool(QObject *parent = nullptr);

    QUrl globalImage(QQuickWindow *window) const;
    void setGlobalImage(QQuickWindow *window, const QUrl &url);

    QRect boundingRect(const QUrl &url) const;
    void setDesktop(QQuickWindow *window, const QUrl &url);
    void unsetDesktop(QQuickWindow *window);

Q_SIGNALS:
    void geometryChanged();
    void desktopWindowChanged(QQuickWindow *sourceWindow);

private:
    std::unordered_map<QQuickWindow *, QUrl /* image path */> m_windowMap;

    mutable std::shared_mutex m_lock;
    QTimer m_geometryChangedTimer;
};
