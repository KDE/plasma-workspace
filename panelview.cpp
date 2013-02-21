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

#include <KGlobal>
#include <KWindowSystem>
#include <kwindoweffects.h>

#include <Plasma/Package>

PanelView::PanelView(Plasma::Corona *corona, QWindow *parent)
    : View(corona, parent),
       m_offset(0),
       m_maxLength(0),
       m_minLength(0)
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
    connect(this, &View::containmentChanged,
            this, &PanelView::restore);
}

PanelView::~PanelView()
{
    if (containment()) {
        config().writeEntry("offset", m_offset);
        config().writeEntry("max", m_maxLength);
        config().writeEntry("min", m_minLength);
        config().writeEntry("size", size());
        containment()->corona()->requestConfigSync();
    }
}

KConfigGroup PanelView::config() const
{
    if (!containment()) {
        return KConfigGroup();
    }
    KConfigGroup views(KGlobal::config(), "PlasmaViews");
    views = KConfigGroup(&views, QString("Panel %1").arg(containment()->id()));

    if (containment()->formFactor() == Plasma::Vertical) {
        return KConfigGroup(&views, "Vertical" + QString::number(screen()->size().height()));
    //treat everything else as horizontal
    } else {
        return KConfigGroup(&views, "Horizontal" + QString::number(screen()->size().width()));
    }
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
        setPosition(s->virtualGeometry().topLeft() + QPoint(m_offset, 0));
        break;
    case Plasma::LeftEdge:
        containment()->setFormFactor(Plasma::Vertical);
        setPosition(s->virtualGeometry().topLeft() + QPoint(0, m_offset));
        break;
    case Plasma::RightEdge:
        containment()->setFormFactor(Plasma::Vertical);
        setPosition(s->virtualGeometry().topRight() - QPoint(width(), 0) + QPoint(0, m_offset));
        break;
    case Plasma::BottomEdge:
    default:
        containment()->setFormFactor(Plasma::Horizontal);
        setPosition(s->virtualGeometry().bottomLeft() - QPoint(0, height()) + QPoint(m_offset, 0));
    }
}

void PanelView::restore()
{
    if (!containment()) {
        return;
    }

    m_offset = config().readEntry<int>("offset", 0);
    m_maxLength = config().readEntry<int>("max", -1);
    m_minLength = config().readEntry<int>("min", -1);

    setMinimumSize(QSize(-1, -1));
    //FIXME: an invalid size doesn't work with QWindows
    setMaximumSize(QSize(10000, 10000));

    if (containment()->formFactor() == Plasma::Vertical) {
        if (m_minLength > 0) {
            setMinimumHeight(m_minLength);
        }
        if (m_maxLength > 0) {
            setMaximumHeight(m_maxLength);
        }
        resize(config().readEntry<QSize>("size", QSize(32, screen()->size().width())));
    } else {
        if (m_minLength > 0) {
            setMinimumWidth(m_minLength);
        }
        if (m_maxLength > 0) {
            setMaximumWidth(m_maxLength);
        }
        resize(config().readEntry<QSize>("size", QSize(screen()->size().height(), 32)));
    }

    positionPanel();
}

#include "moc_panelview.cpp"
