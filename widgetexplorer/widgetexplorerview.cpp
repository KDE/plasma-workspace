/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
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

#include "widgetexplorerview.h"

#include <QQmlContext>
#include <QQmlError>
#include <QQuickItem>

#include <KLocalizedString>
#include <KWindowSystem>
#include <kwindoweffects.h>

#include <Plasma/Containment>

WidgetExplorerView::WidgetExplorerView(const QString &qmlPath, QWindow *parent)
    : QQuickView(parent),
       m_containment(0),
       m_qmlPath(qmlPath),
       m_widgetExplorer(0)
{
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint);
    //KWindowSystem::setType(winId(), NET::Dock);

    //TODO: how to take the shape from the framesvg?
    KWindowEffects::enableBlurBehind(winId(), true);

//     connect(this, &View::locationChanged,
//             this, &WidgetExplorerView::positionPanel);
}

WidgetExplorerView::~WidgetExplorerView()
{
}

void WidgetExplorerView::init()
{
    qDebug() << "Loading WidgetExplorer: " << m_qmlPath;

    m_widgetExplorer = new WidgetExplorer(this);
    m_widgetExplorer->populateWidgetList();
    m_widgetExplorer->setContainment(m_containment);

    rootContext()->setContextProperty("widgetExplorer", m_widgetExplorer);
    setTitle(i18n("Add Widgets"));
    setColor(Qt::transparent);
    setResizeMode(QQuickView::SizeRootObjectToView);
    setSource(QUrl::fromLocalFile(m_qmlPath));

    connect(m_widgetExplorer, &WidgetExplorer::closeClicked, this, &QQuickView::close);
    connect(this, &QQuickView::statusChanged, this, &WidgetExplorerView::widgetExplorerStatusChanged);
    connect(this, &QQuickView::visibleChanged, this, &WidgetExplorerView::widgetExplorerClosed);
}


void WidgetExplorerView::setContainment(Plasma::Containment* c)
{
    m_containment = c;
    m_widgetExplorer->setContainment(c);
}


void WidgetExplorerView::widgetExplorerClosed(bool visible)
{
    if (!visible) {

    }
}

void WidgetExplorerView::widgetExplorerStatusChanged()
{
    foreach (QQmlError e, errors()) {
        qWarning() << "Error in WidgetExplorer: " << e.toString();
    }
}

