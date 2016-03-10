/*
 *   Copyright 2015 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef WAYLANDDIALOGFILTER_H
#define WAYLANDDIALOGFILTER_H

#include <QObject>

#include <QWindow>
#include <QPointer>

namespace KWayland
{
    namespace Client
    {
        class PlasmaShellSurface;
    }
}

class ShellCorona;

class WaylandDialogFilter : public QObject
{
    Q_OBJECT
public:
    WaylandDialogFilter(ShellCorona *c, QWindow *parent = 0);
    ~WaylandDialogFilter() override;

    static void install(QWindow *dialog, ShellCorona *c);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupWaylandIntegration(ShellCorona *c);

    QPointer<QWindow> m_dialog;
    QPointer<KWayland::Client::PlasmaShellSurface> m_shellSurface;
};

#endif
