/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SPLASH_WINDOW_H_
#define SPLASH_WINDOW_H_

#include <KQuickAddons/QuickViewSharedEngine>

class QMouseEvent;
class QKeyEvent;

class SplashWindow : public KQuickAddons::QuickViewSharedEngine
{
public:
    SplashWindow(bool testing, bool window, const QString &theme);

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

#endif // SPLASH_WINDOW_H_
