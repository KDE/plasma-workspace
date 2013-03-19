/*
 *   Copyright 2013 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
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

#include "panelconfigview.h"
#include "panelview.h"

#include <QDebug>
#include <QDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>

#include <KGlobal>
#include <KLocalizedString>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>

//////////////////////////////PanelConfigView
PanelConfigView::PanelConfigView(Plasma::Containment *containment, PanelView *panelView, QWindow *parent)
    : QQuickView(parent),
      m_containment(containment)
{

    //FIXME: problem on nvidia, all windows should be transparent or won't show
    setColor(Qt::transparent);
    setTitle(i18n("%1 Settings", m_containment->title()));


    if (!m_containment->corona()->package().isValid()) {
        qWarning() << "Invalid home screen package";
    }

    setResizeMode(QQuickView::SizeViewToRootObject);

    engine()->rootContext()->setContextProperty("panel", panelView);
    engine()->rootContext()->setContextProperty("configDialog", this);
    setSource(QUrl::fromLocalFile(panelView->corona()->package().filePath("panelconfigurationui")));
}

PanelConfigView::~PanelConfigView()
{
}


//To emulate Qt::WA_DeleteOnClose that QWindow doesn't have
void PanelConfigView::hideEvent(QHideEvent *ev)
{
    QQuickWindow::hideEvent(ev);
    deleteLater();
}

void PanelConfigView::resizeEvent(QResizeEvent *re)
{
    if (!rootObject()) {
        return;
    }
    rootObject()->setWidth(re->size().width());
    rootObject()->setHeight(re->size().height());
    QQuickWindow::resizeEvent(re);
}


#include "moc_panelconfigview.cpp"
