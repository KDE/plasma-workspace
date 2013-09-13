/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
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

#include "desktopview.h"
#include "shellcorona.h"

#include <Plasma/Package>

DesktopView::DesktopView(ShellCorona *corona, QWindow *parent)
    : PlasmaQuickView(corona, parent)
{
    setSource(QUrl::fromLocalFile(corona->package().filePath("views", "Desktop.qml")));
}

DesktopView::~DesktopView()
{
    
}
/*
void DesktopView::showConfigurationInterface(Plasma::Applet *applet)
{
    if (m_configView) {
        m_configView.data()->hide();
        m_configView.data()->deleteLater();
    }

    if (!applet || !applet->containment()) {
        return;
    }

    Plasma::Containment *cont = qobject_cast<Plasma::Containment *>(applet);

    if (cont) {
        m_configView = new PanelConfigView(cont, this);
    } else {
        m_configView = new ConfigView(applet);
    }
    m_configView.data()->init();
    m_configView.data()->show();
}*/


#include "moc_desktopview.cpp"
