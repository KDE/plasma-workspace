/***************************************************************************
 *   Copyright (C) 2008 - 2009 by Lukas Appelhans <l.appelhans@gmx.de>     *
 *   Copyright (C) 2010 - 2011 by Ingomar Wesp <ingomar@wesp.name>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#include "popup.h"

// Qt
#include <Qt>
#include <QtGlobal>
#include <QtCore/QEvent>
#include <QtCore/QMargins>
#include <QtCore/QSize>
#include <QtGui/QGraphicsLayout>
#include <QtGui/QLayout>

// KDE
#include <KIconLoader>

// Plasma
#include <Plasma/Applet>
#include <Plasma/Corona>
#include <Plasma/Dialog>

// Own
#include "popuplauncherlist.h"
#include "quicklaunch.h"

using Plasma::Applet;
using Plasma::Corona;
using Plasma::Dialog;

namespace Quicklaunch {

Popup::Popup(Quicklaunch *applet)
:
    Dialog(0, Qt::X11BypassWindowManagerHint),
    m_applet(applet),
    m_launcherList(new PopupLauncherList())
{
    m_applet->containment()->corona()->addItem(m_launcherList);
    m_launcherList->installEventFilter(this);
    setGraphicsWidget(m_launcherList);

    connect(m_applet, SIGNAL(geometryChanged()), SLOT(onAppletGeometryChanged()));
    connect(m_launcherList, SIGNAL(launcherClicked()), SLOT(onLauncherClicked()));
}

Popup::~Popup()
{
    Dialog::close();
    delete m_launcherList;
}

bool Popup::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched); // watched == m_launcherList

    if (event->type() == QEvent::LayoutRequest) {
        syncSizeAndPosition();
    }
    return false;
}

PopupLauncherList *Popup::launcherList()
{
    return m_launcherList;
}

void Popup::show()
{
    Dialog::show();
    syncSizeAndPosition();
}

void Popup::onAppletGeometryChanged()
{
    move(m_applet->popupPosition(size()));
}

void Popup::onLauncherClicked()
{
    hide();
}

void Popup::syncSizeAndPosition()
{
    const QMargins margins = contentsMargins();

    QSize newSize(
        m_launcherList->preferredWidth() + margins.left() + margins.right(),
        m_launcherList->preferredHeight() + margins.top() + margins.bottom());

    m_launcherList->resize(m_launcherList->preferredWidth(), m_launcherList->preferredHeight());

    syncToGraphicsWidget();
    move(m_applet->popupPosition(size(), Qt::AlignRight));
}
}

#include "popup.moc"
