/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Plasma/Corona>
#include <QPointer>
#include <QWindow>

class QScreen;

class DesktopView : public QWindow
{
    Q_OBJECT

public:
    explicit DesktopView(Plasma::Corona *c, QScreen *targetScreen = nullptr);
    ~DesktopView() override;

    /*This is different from screen() as is always there, even if the window is
      temporarily outside the screen or if is hidden: only plasmashell will ever
      change this property, unlike QWindow::screen()*/
    void setScreenToFollow(QScreen *screen);
    QScreen *screenToFollow() const;

private:
    QPointer<QScreen> m_screenToFollow;
};
