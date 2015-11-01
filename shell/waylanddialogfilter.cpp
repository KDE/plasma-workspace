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
#include <QDebug>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>


class DialogShadows : public PanelShadows {
public:
    explicit DialogShadows(QObject *parent = 0)
        : PanelShadows(parent, QStringLiteral("dialogs/background"))
    {}

    static DialogShadows *self();
};

class DialogShadowsSingleton
{
public:
    DialogShadowsSingleton()
    {
    }

   DialogShadows self;
};

Q_GLOBAL_STATIC(DialogShadowsSingleton, privateDialogShadowsSelf)

DialogShadows *DialogShadows::self()
{
    return &privateDialogShadowsSelf->self;
}



WaylandDialogFilter::WaylandDialogFilter(ShellCorona *c, QWindow *parent)
    : QObject(parent),
      m_dialog(parent)
{
    parent->installEventFilter(this);
    setupWaylandIntegration(c);
}

WaylandDialogFilter::~WaylandDialogFilter()
{
    DialogShadows::self()->removeWindow(m_dialog);
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
            Plasma::FrameSvg::EnabledBorders enabledBorders = Plasma::FrameSvg::AllBorders;
            Plasma::FrameSvg *background = m_dialog->property("__plasma_frameSvg").value<Plasma::FrameSvg *>();
            if (background) {
                enabledBorders = background->enabledBorders();
            }
            DialogShadows::self()->addWindow(m_dialog, enabledBorders);
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
