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

#include "waylanddialogfilter.h"
#include "shellcorona.h"
#include "panelshadows_p.h"

#include <QMoveEvent>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>


WaylandDialogFilter::WaylandDialogFilter(ShellCorona *c, QWindow *parent)
    : QObject(parent),
      m_dialog(parent)
{
    parent->installEventFilter(this);
    setupWaylandIntegration(c);
}

WaylandDialogFilter::~WaylandDialogFilter()
{
    PanelShadows::self()->removeWindow(m_dialog);
}

void WaylandDialogFilter::install(QWindow *dialog, ShellCorona *c)
{
    new WaylandDialogFilter(c, dialog);
}

bool WaylandDialogFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Move) {
        QMoveEvent *me = static_cast<QMoveEvent *>(event);
        if (m_shellSurface) {
            m_shellSurface->setPosition(me->pos());
        }
    } else if (event->type() == QEvent::Show) {
        if (m_dialog == watched) {
            PanelShadows::self()->addWindow(m_dialog);
        }
    }
    return QObject::eventFilter(watched, event);
}

void WaylandDialogFilter::setupWaylandIntegration(ShellCorona *c)
{
    if (m_shellSurface) {
        // already setup
        return;
    }
    if (c) {
        using namespace KWayland::Client;
        PlasmaShell *interface = c->waylandPlasmaShellInterface();
        if (!interface) {
            return;
        }
        Surface *s = Surface::fromWindow(m_dialog);
        if (!s) {
            return;
        }
        m_shellSurface = interface->createSurface(s, this);
    }
}

#include "moc_waylanddialogfilter.cpp"
