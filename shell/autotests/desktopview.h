/*
 *  Copyright 2016 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DESKTOPVIEW_H
#define DESKTOPVIEW_H

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

#endif // DESKTOPVIEW_H
