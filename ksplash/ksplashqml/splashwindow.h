/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SPLASH_WINDOW_H_
#define SPLASH_WINDOW_H_

#include <KQuickAddons/QuickViewSharedEngine>

class QMouseEvent;
class QKeyEvent;

namespace KWayland
{
namespace Client
{
class PlasmaShellSurface;
}
}

class SplashWindow : public KQuickAddons::QuickViewSharedEngine
{
public:
    SplashWindow(bool testing, bool window, const QString &theme);

    void setStage(int stage);
    virtual void setGeometry(const QRect &rect);

protected:
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupWaylandIntegration();
    int m_stage;
    const bool m_testing;
    const bool m_window;
    const QString m_theme;
    KWayland::Client::PlasmaShellSurface *m_shellSurface = nullptr;
};

#endif // SPLASH_WINDOW_H_
