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

#include "panelview.h"

#include <QDebug>
#include <QScreen>

#include <KWindowSystem>
#include <kwindoweffects.h>

#include <Plasma/Package>

PanelView::PanelView(Plasma::Corona *corona, QWindow *parent)
    : View(corona, parent)
{
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint);
    KWindowSystem::setType(winId(), NET::Dock);

    //TODO: how to take the shape from the framesvg?
    KWindowEffects::enableBlurBehind(winId(), true);

    //Screen management
    connect(screen(), &QScreen::virtualGeometryChanged,
            this, &PanelView::positionPanel);
    connect(this, &View::locationChanged,
            this, &PanelView::positionPanel);
}

PanelView::~PanelView()
{
    
}

void PanelView::init()
{
    if (!corona()->package().isValid()) {
        qWarning() << "Invalid home screen package";
    }

    setResizeMode(View::SizeRootObjectToView);
    setSource(QUrl::fromLocalFile(corona()->package().filePath("ui", "PanelView.qml")));
}

void PanelView::positionPanel()
{
    if (!containment()) {
        return;
    }

    QScreen *s = screen();
    
    switch (containment()->location()) {
    case Plasma::TopEdge:
        containment()->setFormFactor(Plasma::Horizontal);
        setPosition(s->virtualGeometry().topLeft());
        break;
    case Plasma::LeftEdge:
        containment()->setFormFactor(Plasma::Vertical);
        setPosition(s->virtualGeometry().topLeft());
        break;
    case Plasma::RightEdge:
        containment()->setFormFactor(Plasma::Vertical);
        setPosition(s->virtualGeometry().topRight() - QPoint(width(), 0));
        break;
    case Plasma::BottomEdge:
    default:
        containment()->setFormFactor(Plasma::Horizontal);
        setPosition(s->virtualGeometry().bottomLeft() - QPoint(0, height()));
    }
}

#include "moc_panelview.cpp"
