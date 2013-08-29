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
#include "desktopcorona.h"
#include "panelshadows_p.h"

#include <QAction>
#include <QDebug>
#include <QScreen>
#include <QQmlEngine>
#include <QQmlContext>

#include <KActionCollection>
#include <KWindowSystem>
#include <kwindoweffects.h>

#include <Plasma/Containment>
#include <Plasma/Package>

PanelView::PanelView(DesktopCorona *corona, QWindow *parent)
    : View(corona, parent),
       m_offset(0),
       m_maxLength(0),
       m_minLength(0),
       m_alignment(Qt::AlignLeft),
       m_corona(corona)
{
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus);
    KWindowSystem::setType(winId(), NET::Dock);
    setVisible(false);

    //TODO: how to take the shape from the framesvg?
    KWindowEffects::enableBlurBehind(winId(), true);

    //Screen management
    connect(screen(), &QScreen::virtualGeometryChanged,
            this, &PanelView::positionPanel);
    connect(this, &View::locationChanged,
            this, &PanelView::positionPanel);
    connect(this, &View::containmentChanged,
            this, &PanelView::restore);

    if (!m_corona->package().isValid()) {
        qWarning() << "Invalid home screen package";
    }

    setResizeMode(View::SizeRootObjectToView);
    qmlRegisterType<QScreen>();
    engine()->rootContext()->setContextProperty("panel", this);
    setSource(QUrl::fromLocalFile(m_corona->package().filePath("views", "Panel.qml")));
    positionPanel();
    PanelShadows::self()->addWindow(this);
}

PanelView::~PanelView()
{
    if (containment()) {
        config().writeEntry("offset", m_offset);
        config().writeEntry("max", m_maxLength);
        config().writeEntry("min", m_minLength);
        if (formFactor() == Plasma::Types::Vertical) {
            config().writeEntry("length", size().height());
            config().writeEntry("thickness", size().width());
        } else {
            config().writeEntry("length", size().width());
            config().writeEntry("thickness", size().height());
        }
        config().writeEntry("alignment", (int)m_alignment);
        m_corona->requestApplicationConfigSync();
        m_corona->requestApplicationConfigSync();
    }
    PanelShadows::self()->removeWindow(this);
}

KConfigGroup PanelView::config() const
{
    if (!containment()) {
        return KConfigGroup();
    }
    KConfigGroup views(m_corona->applicationConfig(), "PlasmaViews");
    views = KConfigGroup(&views, QString("Panel %1").arg(containment()->id()));

    if (containment()->formFactor() == Plasma::Types::Vertical) {
        return KConfigGroup(&views, "Types::Vertical" + QString::number(screen()->size().height()));
    //treat everything else as horizontal
    } else {
        return KConfigGroup(&views, "Horizontal" + QString::number(screen()->size().width()));
    }
}

Qt::Alignment PanelView::alignment() const
{
    return m_alignment;
}

void PanelView::setAlignment(Qt::Alignment alignment)
{
    if (m_alignment == alignment) {
        return;
    }

    m_alignment = alignment;
    config().writeEntry("alignment", (int)m_alignment);
    emit alignmentChanged();
    positionPanel();
}

int PanelView::offset() const
{
    return m_offset;
}

void PanelView::setOffset(int offset)
{
    if (m_offset == offset) {
        return;
    }

    m_offset = offset;
    config().writeEntry("offset", m_offset);
    positionPanel();
    emit offsetChanged();
    m_corona->requestApplicationConfigSync();
}

int PanelView::thickness() const
{
    return config().readEntry<int>("thickness", 30);
}

void PanelView::setThickness(int value)
{
    if (value == thickness()) {
        return;
    }

    if (formFactor() == Plasma::Types::Vertical) {
        setWidth(value);
    } else {
        setHeight(value);
    }
    config().writeEntry("thickness", value);
    emit thicknessChanged();
    m_corona->requestApplicationConfigSync();
}

int PanelView::length() const
{
    if (formFactor() == Plasma::Types::Vertical) {
        return config().readEntry<int>("length", screen()->size().height());
    } else {
        return config().readEntry<int>("length", screen()->size().width());
    }
}

void PanelView::setLength(int value)
{
    if (value == length()) {
        return;
    }

    if (formFactor() == Plasma::Types::Vertical) {
        setHeight(value);
    } else {
        setWidth(value);
    }
    config().writeEntry("length", value);
    emit lengthChanged();
    m_corona->requestApplicationConfigSync();
}

int PanelView::maximumLength() const
{
    return m_maxLength;
}

void PanelView::setMaximumLength(int length)
{
    if (length == m_maxLength) {
        return;
    }

    if (m_minLength > length) {
        setMinimumLength(length);
    }

    if (formFactor() == Plasma::Types::Vertical) {
        setMaximumHeight(length);
    } else {
        setMaximumWidth(length);
    }
    config().writeEntry("maxLength", length);
    m_maxLength = length;
    emit maximumLengthChanged();
    positionPanel();
    m_corona->requestApplicationConfigSync();
}

int PanelView::minimumLength() const
{
    return m_minLength;
}

void PanelView::setMinimumLength(int length)
{
    if (length == m_minLength) {
        return;
    }

    if (m_maxLength < length) {
        setMaximumLength(length);
    }

    if (formFactor() == Plasma::Types::Vertical) {
        setMinimumHeight(length);
    } else {
        setMinimumWidth(length);
    }
    config().writeEntry("minLength", length);
    m_minLength = length;
    positionPanel();
    emit minimumLengthChanged();
    m_corona->requestApplicationConfigSync();
}

void PanelView::positionPanel()
{
    if (!containment()) {
        return;
    }

    QScreen *s = screen();
    const int oldThickness = thickness();

    switch (containment()->location()) {
    case Plasma::Types::TopEdge:
        containment()->setFormFactor(Plasma::Types::Horizontal);
        restore();

        switch (m_alignment) {
        case Qt::AlignCenter:
            setPosition(QPoint(s->virtualGeometry().center().x(), s->virtualGeometry().top()) + QPoint(m_offset - size().width()/2, 0));
            break;
        case Qt::AlignRight:
            setPosition(s->virtualGeometry().topRight() - QPoint(m_offset + size().width(), 0));
            break;
        case Qt::AlignLeft:
        default:
            setPosition(s->virtualGeometry().topLeft() + QPoint(m_offset, 0));
        }
        break;

    case Plasma::Types::LeftEdge:
        containment()->setFormFactor(Plasma::Types::Vertical);
        restore();
        switch (m_alignment) {
        case Qt::AlignCenter:
            setPosition(QPoint(s->virtualGeometry().left(), s->virtualGeometry().center().y()) + QPoint(0, m_offset));
            break;
        case Qt::AlignRight:
            setPosition(s->virtualGeometry().bottomLeft() - QPoint(0, m_offset + size().height()));
            break;
        case Qt::AlignLeft:
        default:
            setPosition(s->virtualGeometry().topLeft() + QPoint(0, m_offset));
        }
        break;

    case Plasma::Types::RightEdge:
        containment()->setFormFactor(Plasma::Types::Vertical);
        restore();
        switch (m_alignment) {
        case Qt::AlignCenter:
            setPosition(QPoint(s->virtualGeometry().right(), s->virtualGeometry().center().y()) - QPoint(width(), 0) + QPoint(0, m_offset - size().height()/2));
            break;
        case Qt::AlignRight:
            setPosition(s->virtualGeometry().bottomRight() - QPoint(width(), 0) - QPoint(0, m_offset + size().height()));
            break;
        case Qt::AlignLeft:
        default:
            setPosition(s->virtualGeometry().topRight() - QPoint(width(), 0) + QPoint(0, m_offset));
        }
        break;

    case Plasma::Types::BottomEdge:
    default:
        containment()->setFormFactor(Plasma::Types::Horizontal);
        restore();
        switch (m_alignment) {
        case Qt::AlignCenter:
            setPosition(QPoint(s->virtualGeometry().center().x(), s->virtualGeometry().bottom()) + QPoint(m_offset - size().width()/2, 0));
            break;
        case Qt::AlignRight:
            setPosition(s->virtualGeometry().bottomRight() - QPoint(0, height()) - QPoint(m_offset + size().width(), 0));
            break;
        case Qt::AlignLeft:
        default:
            setPosition(s->virtualGeometry().bottomLeft() - QPoint(0, height()) + QPoint(m_offset, 0));
        }
    }
    if (thickness() != oldThickness) {
        emit thicknessChanged();
    }
}

void PanelView::restore()
{
    if (!containment()) {
        return;
    }

    setVisible(containment()->isUiReady());
    connect(containment(), &Plasma::Containment::uiReadyChanged,
            this, &PanelView::setVisible);

    static const int MINSIZE = 10;

    m_offset = config().readEntry<int>("offset", 0);
    if (m_alignment != Qt::AlignCenter) {
        m_offset = qMax(0, m_offset);
    }

    m_maxLength = config().readEntry<int>("maxLength", -1);
    m_minLength = config().readEntry<int>("minLength", -1);
    m_alignment = (Qt::Alignment)config().readEntry<int>("alignment", Qt::AlignLeft);

    setMinimumSize(QSize(-1, -1));
    //FIXME: an invalid size doesn't work with QWindows
    setMaximumSize(screen()->size());

    if (containment()->formFactor() == Plasma::Types::Vertical) {
        const int maxSize = screen()->size().height() - m_offset;
        m_maxLength = qBound<int>(MINSIZE, m_maxLength, maxSize);
        m_minLength = qBound<int>(MINSIZE, m_minLength, maxSize);

        resize(config().readEntry<int>("thickness", 32),
               qBound<int>(MINSIZE, config().readEntry<int>("length", screen()->size().height()), maxSize));

        setMinimumHeight(m_minLength);
        setMaximumHeight(m_maxLength);

    //Horizontal
    } else {
        const int maxSize = screen()->size().width() - m_offset;
        m_maxLength = qBound<int>(MINSIZE, m_maxLength, maxSize);
        m_minLength = qBound<int>(MINSIZE, m_minLength, maxSize);

        resize(qBound<int>(MINSIZE, config().readEntry<int>("length", screen()->size().height()), maxSize),
               config().readEntry<int>("thickness", 32));

        setMinimumWidth(m_minLength);
        setMaximumWidth(m_maxLength);
    }

    emit maximumLengthChanged();
    emit minimumLengthChanged();
    emit offsetChanged();
    emit alignmentChanged();
}

void PanelView::resizeEvent(QResizeEvent *ev)
{
    if (containment()->formFactor() == Plasma::Types::Vertical) {
        config().writeEntry("length", ev->size().height());
        config().writeEntry("thickness", ev->size().width());
        if (ev->size().height() != ev->oldSize().height()) {
            emit lengthChanged();
        }
        if (ev->size().width() != ev->oldSize().width()) {
            emit thicknessChanged();
        }
    } else {
        config().writeEntry("length", ev->size().width());
        config().writeEntry("thickness", ev->size().height());
        if (ev->size().width() != ev->oldSize().width()) {
            emit lengthChanged();
        }
        if (ev->size().height() != ev->oldSize().height()) {
            emit thicknessChanged();
        }
    }

    View::resizeEvent(ev);
}

void PanelView::showEvent(QShowEvent *event)
{
    PanelShadows::self()->addWindow(this);
    View::showEvent(event);
}

#include "moc_panelview.cpp"
