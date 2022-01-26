/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QScreen>

#include <KQuickAddons/QuickViewSharedEngine>

class QMouseEvent;
class QKeyEvent;

class SplashWindow : public KQuickAddons::QuickViewSharedEngine
{
public:
    SplashWindow(bool testing, bool window, const QString &theme, QScreen *screen);

    void setStage(int stage);
    virtual void setGeometry(const QRect &rect);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    int m_stage;
    const bool m_testing;
    const bool m_window;
    const QString m_theme;
};
