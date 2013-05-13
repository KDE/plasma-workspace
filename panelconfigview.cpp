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
#include <QScreen>

#include <KGlobal>
#include <KLocalizedString>
#include <kwindoweffects.h>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>

//////////////////////////////PanelConfigView
PanelConfigView::PanelConfigView(Plasma::Containment *containment, PanelView *panelView, QWindow *parent)
    : ConfigView(containment, parent),
      m_containment(containment),
      m_panelView(panelView)
{

    setFlags(Qt::FramelessWindowHint);

    KWindowEffects::enableBlurBehind(winId(), true);

    engine()->rootContext()->setContextProperty("panel", panelView);
    engine()->rootContext()->setContextProperty("configDialog", this);
    connect(containment, &Plasma::Containment::formFactorChanged,
            this, &PanelConfigView::syncGeometry);
}

PanelConfigView::~PanelConfigView()
{
}

void PanelConfigView::init()
{
    setSource(QUrl::fromLocalFile(m_containment->corona()->package().filePath("panelconfigurationui")));
    syncGeometry();
}

void PanelConfigView::syncGeometry()
{
    if (!m_containment) {
        return;
    }

    if (m_containment->formFactor() == Plasma::Types::Vertical) {
        resize(rootObject()->implicitWidth(), screen()->size().height());

        if (m_containment->location() == Plasma::Types::LeftEdge) {
            setPosition(screen()->geometry().left() + m_panelView->thickness(), screen()->geometry().top());
        } else if (m_containment->location() == Plasma::Types::RightEdge) {
            setPosition(screen()->geometry().right() - width() - m_panelView->thickness(), screen()->geometry().top());
        }

    } else {
        resize(screen()->size().width(), rootObject()->implicitHeight());

        if (m_containment->location() == Plasma::Types::TopEdge) {
            setPosition(screen()->geometry().left(), screen()->geometry().top() + m_panelView->thickness());
        } else if (m_containment->location() == Plasma::Types::BottomEdge) {
            setPosition(screen()->geometry().left(), screen()->geometry().bottom() - height() - m_panelView->thickness());
        }
    }
}

void PanelConfigView::focusOutEvent(QFocusEvent *ev)
{
    Q_UNUSED(ev)
    close();
}

#include "moc_panelconfigview.cpp"
