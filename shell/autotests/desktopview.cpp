/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "desktopview.h"

#include <QScreen>

DesktopView::DesktopView(Plasma::Corona *c, QScreen *targetScreen)
    : QWindow(targetScreen)
{
    if (targetScreen) {
        setScreenToFollow(targetScreen);
        setScreen(targetScreen);
        setGeometry(targetScreen->geometry());
    }
}

DesktopView::~DesktopView()
{
}

void DesktopView::setScreenToFollow(QScreen *screen)
{
    if (screen == m_screenToFollow) {
        return;
    }

    m_screenToFollow = screen;
    setScreen(screen);
}

QScreen *DesktopView::screenToFollow() const
{
    return m_screenToFollow;
}
