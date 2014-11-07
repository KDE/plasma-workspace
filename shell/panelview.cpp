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

#include <config-plasma.h>

#include "panelview.h"
#include "shellcorona.h"
#include "panelshadows_p.h"
#include "panelconfigview.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QScreen>
#include <QQmlEngine>
#include <QQmlContext>
#include <QTimer>

#include <kactioncollection.h>
#include <kwindowsystem.h>
#include <kwindoweffects.h>

#include <Plasma/Containment>
#include <Plasma/Package>
#include <KScreen/Config>

#if HAVE_X11
#include <xcb/xcb.h>
#include <QX11Info>
#endif

static const int MINSIZE = 10;

PanelView::PanelView(ShellCorona *corona, QScreen *targetScreen, QWindow *parent)
    : PlasmaQuick::View(corona, parent),
       m_offset(0),
       m_maxLength(0),
       m_minLength(0),
       m_distance(0),
       m_thickness(30),
       m_alignment(Qt::AlignLeft),
       m_corona(corona),
       m_visibilityMode(NormalPanel),
       m_background(0)
{
    if (targetScreen) {
        setScreen(targetScreen);
    }
    setResizeMode(QQuickView::SizeRootObjectToView);
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus);

    themeChanged();
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PanelView::themeChanged);

    m_positionPaneltimer.setSingleShot(true);
    m_positionPaneltimer.setInterval(150);
    connect(&m_positionPaneltimer, &QTimer::timeout,
            this, [=] () {
                restore();
                positionPanel();
            });

    m_unhideTimer.setSingleShot(true);
    m_unhideTimer.setInterval(500);
    connect(&m_unhideTimer, &QTimer::timeout,
            this, &PanelView::restoreAutoHide);

    connect(screen(), SIGNAL(geometryChanged(QRect)),
            &m_positionPaneltimer, SLOT(start()));
    connect(this, SIGNAL(locationChanged(Plasma::Types::Location)),
            &m_positionPaneltimer, SLOT(start()));
    connect(this, SIGNAL(containmentChanged()),
            this, SLOT(containmentChanged()));
    connect(this, &View::locationChanged, [=] () {
                emit m_corona->availableScreenRectChanged();
                emit m_corona->availableScreenRegionChanged();
            });

    if (!m_corona->package().isValid()) {
        qWarning() << "Invalid home screen package";
    }

    m_strutsTimer.setSingleShot(true);
    connect(&m_strutsTimer, &QTimer::timeout,
            this, &PanelView::updateStruts);
    connect(m_corona->screensConfiguration(), &KScreen::Config::outputAdded,
            this, &PanelView::updateStruts);
    connect(m_corona->screensConfiguration(), &KScreen::Config::outputRemoved,
            this, &PanelView::updateStruts);

    qmlRegisterType<QScreen>();
    engine()->rootContext()->setContextProperty("panel", this);
    setSource(QUrl::fromLocalFile(m_corona->package().filePath("views", "Panel.qml")));
    PanelShadows::self()->addWindow(this);
}

PanelView::~PanelView()
{
    if (containment()) {
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

    if (formFactor() == Plasma::Types::Vertical) {
        return KConfigGroup(&views, "Vertical" + QString::number(screen()->size().height()));
    //treat everything else as horizontal
    } else {
        return KConfigGroup(&views, "Horizontal" + QString::number(screen()->size().width()));
    }
}

void PanelView::maximize()
{
    int length;
    if (containment()->formFactor() == Plasma::Types::Vertical) {
        length = screen()->size().height();
    } else {
        length = screen()->size().width();
    }
    setOffset(0);
    setMinimumLength(length);
    setMaximumLength(length);
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

    if (formFactor() == Plasma::Types::Vertical) {
        if (offset + m_maxLength > screen()->size().height()) {
            setMaximumLength( -m_offset + screen()->size().height() );
        }
    } else {
        if (offset + m_maxLength > screen()->size().width()) {
            setMaximumLength( -m_offset + screen()->size().width() );
        }
    }

    m_offset = offset;
    config().writeEntry("offset", m_offset);
    positionPanel();
    emit offsetChanged();
    m_corona->requestApplicationConfigSync();
    m_strutsTimer.start(STRUTSTIMERDELAY);
    emit m_corona->availableScreenRegionChanged();
}

int PanelView::thickness() const
{
    return m_thickness;
}

void PanelView::setThickness(int value)
{
    if (value == thickness()) {
        return;
    }

    m_thickness = value;
    if (formFactor() == Plasma::Types::Vertical) {
        setMinimumWidth(value);
        setMaximumWidth(value);
    } else {
        setMinimumHeight(value);
        setMaximumHeight(value);
    }
    config().writeEntry("thickness", value);
    m_corona->requestApplicationConfigSync();
    positionPanel();
    emit m_corona->availableScreenRectChanged();
    emit m_corona->availableScreenRegionChanged();
}

int PanelView::length() const
{
    int defaultLength = formFactor() == Plasma::Types::Vertical ? screen()->size().height() : screen()->size().width();
    return config().isValid() ? config().readEntry<int>("length", defaultLength) : defaultLength;
}

void PanelView::setLength(int value)
{
    if (value == length()) {
        return;
    }

    config().writeEntry("length", value);
    m_corona->requestApplicationConfigSync();
    positionPanel();


    const int maxSize = screen()->size().width() - m_offset;
    if (containment()->formFactor() == Plasma::Types::Vertical) {
        resize(thickness(), qBound<int>(MINSIZE, value, maxSize));
    //Horizontal
    } else {
        resize(qBound<int>(MINSIZE, value, maxSize), thickness());
    }
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
    m_corona->requestApplicationConfigSync();

    positionPanel();
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
    emit minimumLengthChanged();
    m_corona->requestApplicationConfigSync();

    positionPanel();
}

int PanelView::distance() const
{
    return m_distance;
}

void PanelView::setDistance(int dist)
{
    if (m_distance == dist) {
        return;
    }

    m_distance = dist;
    emit distanceChanged();
    positionPanel();
}

void PanelView::setVisibilityMode(PanelView::VisibilityMode mode)
{
    m_visibilityMode = mode;

    if (mode == LetWindowsCover) {
        KWindowSystem::setState(winId(), NET::KeepBelow);
    } else {
        KWindowSystem::clearState(winId(), NET::KeepBelow);
    }

    disconnect(containment(), &Plasma::Applet::activated, this, &PanelView::showTemporarily);
    if (!(mode == NormalPanel || mode == WindowsGoBelow || mode == AutoHide)) {
        connect(containment(), &Plasma::Applet::activated, this, &PanelView::showTemporarily);
    }

    if (config().isValid())
        config().writeEntry("panelVisibility", (int)mode);

    updateStruts();

    emit visibilityModeChanged();
    restoreAutoHide();
}

PanelView::VisibilityMode PanelView::visibilityMode() const
{
    return m_visibilityMode;
}

void PanelView::positionPanel()
{
    if (!containment()) {
        return;
    }

    KWindowEffects::SlideFromLocation slideLocation = KWindowEffects::NoEdge;

    switch (containment()->location()) {
    case Plasma::Types::TopEdge:
        containment()->setFormFactor(Plasma::Types::Horizontal);
        slideLocation = KWindowEffects::TopEdge;
        break;

    case Plasma::Types::LeftEdge:
        containment()->setFormFactor(Plasma::Types::Vertical);
        slideLocation = KWindowEffects::LeftEdge;
        break;

    case Plasma::Types::RightEdge:
        containment()->setFormFactor(Plasma::Types::Vertical);
        slideLocation = KWindowEffects::RightEdge;
        break;

    case Plasma::Types::BottomEdge:
    default:
        containment()->setFormFactor(Plasma::Types::Horizontal);
        slideLocation = KWindowEffects::BottomEdge;
        break;
    }
    m_strutsTimer.stop();
    m_strutsTimer.start(STRUTSTIMERDELAY);
    setMinimumSize(QSize(0, 0));
    setMaximumSize(screen()->size());

    if (formFactor() == Plasma::Types::Vertical) {
        setMinimumSize(QSize(thickness(), m_minLength));
        setMaximumSize(QSize(thickness(), m_maxLength));

        emit thicknessChanged();
        emit lengthChanged();
    } else {
        setMinimumSize(QSize(m_minLength, thickness()));
        setMaximumSize(QSize(m_maxLength, thickness()));

        emit thicknessChanged();
        emit lengthChanged();
    }

    setPosition(geometryByDistance(m_distance).topLeft());

    KWindowEffects::slideWindow(winId(), slideLocation, -1);
}

QRect PanelView::geometryByDistance(int distance) const
{
    QScreen *s = screen();
    QPoint position;
    switch (containment()->location()) {
    case Plasma::Types::TopEdge:
        switch (m_alignment) {
        case Qt::AlignCenter:
            position = QPoint(QPoint(s->geometry().center().x(), s->geometry().top()) + QPoint(m_offset - size().width()/2, distance));
            break;
        case Qt::AlignRight:
            position = QPoint(s->geometry().topRight() - QPoint(m_offset + size().width(), distance));
            break;
        case Qt::AlignLeft:
        default:
            position = QPoint(s->geometry().topLeft() + QPoint(m_offset, distance));
        }
        break;

    case Plasma::Types::LeftEdge:
        switch (m_alignment) {
        case Qt::AlignCenter:
            position = QPoint(QPoint(s->geometry().left(), s->geometry().center().y()) + QPoint(distance, m_offset - size().height()/2));
            break;
        case Qt::AlignRight:
            position = QPoint(s->geometry().bottomLeft() - QPoint(distance, m_offset + size().height()));
            break;
        case Qt::AlignLeft:
        default:
            position = QPoint(s->geometry().topLeft() + QPoint(distance, m_offset));
        }
        break;

    case Plasma::Types::RightEdge:
        switch (m_alignment) {
        case Qt::AlignCenter:
            position = QPoint(QPoint(s->geometry().right(), s->geometry().center().y()) - QPoint(thickness() + distance, 0) + QPoint(0, m_offset - size().height()/2));
            break;
        case Qt::AlignRight:
            position = QPoint(s->geometry().bottomRight() - QPoint(thickness() + distance, 0) - QPoint(0, m_offset + size().height()));
            break;
        case Qt::AlignLeft:
        default:
            position = QPoint(s->geometry().topRight() - QPoint(thickness() + distance, 0) - QPoint(0, m_offset));
        }
        break;

    case Plasma::Types::BottomEdge:
    default:
        switch (m_alignment) {
        case Qt::AlignCenter:
            position = QPoint(QPoint(s->geometry().center().x(), s->geometry().bottom() - thickness() - distance) + QPoint(m_offset - size().width()/2, 1));
            break;
        case Qt::AlignRight:
            position = QPoint(s->geometry().bottomRight() - QPoint(0, thickness() + distance) - QPoint(m_offset + size().width(), 1));
            break;
        case Qt::AlignLeft:
        default:
            position = QPoint(s->geometry().bottomLeft() - QPoint(0, thickness() + distance) + QPoint(m_offset, 1));
        }
    }
    QRect ret = formFactor() == Plasma::Types::Vertical ? QRect(position, QSize(thickness(), length())) : QRect(position, QSize(length(), thickness()));
    ret = ret.intersect(s->geometry());
    return ret;
}

void PanelView::restore()
{
    if (!containment()) {
        return;
    }

    m_offset = config().readEntry<int>("offset", 0);
    if (m_alignment != Qt::AlignCenter) {
        m_offset = qMax(0, m_offset);
    }

    setAlignment((Qt::Alignment)config().readEntry<int>("alignment", Qt::AlignLeft));
    setThickness(config().readEntry<int>("thickness", 30));

    setMinimumSize(QSize(-1, -1));
    //FIXME: an invalid size doesn't work with QWindows
    setMaximumSize(screen()->size());

    if (containment()->formFactor() == Plasma::Types::Vertical) {
        m_maxLength = config().readEntry<int>("maxLength", screen()->size().height());
        m_minLength = config().readEntry<int>("minLength", screen()->size().height());

        const int maxSize = screen()->size().height() - m_offset;
        m_maxLength = qBound<int>(MINSIZE, m_maxLength, maxSize);
        m_minLength = qBound<int>(MINSIZE, m_minLength, maxSize);

        resize(thickness(), qBound<int>(MINSIZE, length(), maxSize));

        setMinimumHeight(m_minLength);
        setMaximumHeight(m_maxLength);

    //Horizontal
    } else {
        m_maxLength = config().readEntry<int>("maxLength", screen()->size().width());
        m_minLength = config().readEntry<int>("minLength", screen()->size().width());

        const int maxSize = screen()->size().width() - m_offset;
        m_maxLength = qBound<int>(MINSIZE, m_maxLength, maxSize);
        m_minLength = qBound<int>(MINSIZE, m_minLength, maxSize);

        resize(qBound<int>(MINSIZE, length(), maxSize), thickness());

        setMinimumWidth(m_minLength);
        setMaximumWidth(m_maxLength);
    }

    setVisibilityMode((VisibilityMode)config().readEntry<int>("panelVisibility", (int)NormalPanel));

    emit maximumLengthChanged();
    emit minimumLengthChanged();
    emit offsetChanged();
    emit alignmentChanged();
}

void PanelView::showConfigurationInterface(Plasma::Applet *applet)
{
    if (!applet || !applet->containment()) {
        return;
    }

    Plasma::Containment *cont = qobject_cast<Plasma::Containment *>(applet);

    if (m_panelConfigView && cont && cont->isContainment()) {
        if (m_panelConfigView.data()->isVisible()) {
            m_panelConfigView.data()->hide();
        } else {
            m_panelConfigView.data()->show();
            KWindowSystem::setState(m_panelConfigView.data()->winId(), NET::SkipTaskbar | NET::SkipPager);
        }
        return;
    } else if (m_panelConfigView) {
        if (m_panelConfigView->applet() == applet) {
            m_panelConfigView->show();
            return;
        } else {
            m_panelConfigView->hide();
            m_panelConfigView->deleteLater();
        }
    }

    if (cont && cont->isContainment()) {
        m_panelConfigView = new PanelConfigView(cont, this);
    } else {
        m_panelConfigView = new PlasmaQuick::ConfigView(applet);
    }

    m_panelConfigView.data()->init();
    m_panelConfigView.data()->show();

    if (cont && cont->isContainment()) {
        KWindowSystem::setState(m_panelConfigView.data()->winId(), NET::SkipTaskbar | NET::SkipPager);
    }
}

void PanelView::restoreAutoHide()
{
    setAutoHideEnabled(m_visibilityMode == AutoHide && (!containment() || containment()->status() < Plasma::Types::RequiresAttentionStatus));
}

void PanelView::setAutoHideEnabled(bool enabled)
{
#if HAVE_X11
    xcb_connection_t *c = QX11Info::connection();
    if (!c) {
        return;
    }

    const QByteArray effectName = QByteArrayLiteral("_KDE_NET_WM_SCREEN_EDGE_SHOW");
    xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom_unchecked(c, false, effectName.length(), effectName.constData());

    QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> atom(xcb_intern_atom_reply(c, atomCookie, NULL));

    if (!atom) {
        return;
    }

    if (!enabled) {
        xcb_delete_property(c, winId(), atom->atom);
        return;
    }

    KWindowEffects::SlideFromLocation slideLocation = KWindowEffects::NoEdge;
    uint32_t value = 0;

    switch (location()) {
    case Plasma::Types::TopEdge:
        value = 0;
        slideLocation = KWindowEffects::TopEdge;
        break;
    case Plasma::Types::RightEdge:
        value = 1;
        slideLocation = KWindowEffects::RightEdge;
        break;
    case Plasma::Types::BottomEdge:
        value = 2;
        slideLocation = KWindowEffects::BottomEdge;
        break;
    case Plasma::Types::LeftEdge:
        value = 3;
        slideLocation = KWindowEffects::LeftEdge;
        break;
    case Plasma::Types::Floating:
    default:
        value = 4;
        break;
    }

    xcb_change_property(c, XCB_PROP_MODE_REPLACE, winId(), atom->atom, XCB_ATOM_CARDINAL, 32, 1, &value);
    KWindowEffects::slideWindow(winId(), slideLocation, -1);
#endif
}

void PanelView::resizeEvent(QResizeEvent *ev)
{
    updateMask();
    //don't setGeometry() to meke really sure we aren't doing a resize loop
    setPosition(geometryByDistance(m_distance).topLeft());
    PlasmaQuick::View::resizeEvent(ev);
}

void PanelView::moveEvent(QMoveEvent *ev)
{
    updateMask();
    PlasmaQuick::View::moveEvent(ev);
}

void PanelView::integrateScreen()
{
    KWindowSystem::setOnAllDesktops(winId(), true);
    KWindowSystem::setType(winId(), NET::Dock);
    setVisibilityMode(m_visibilityMode);

    containment()->reactToScreenChange();
}

void PanelView::showEvent(QShowEvent *event)
{
    PanelShadows::self()->addWindow(this);
    PlasmaQuick::View::showEvent(event);
    integrateScreen();

    //When the screen is set, the screen is recreated internally, so we need to
    //set anything that depends on the winId()
    connect(this, &QWindow::screenChanged, this, [=](QScreen* screen) {
        emit screenChangedProxy(screen);

        if (!screen)
            return;
        integrateScreen();
        showTemporarily();
        m_positionPaneltimer.start();
    });
}

bool PanelView::event(QEvent *e)
{
    if (m_visibilityMode == AutoHide) {
        if (e->type() == QEvent::Enter) {
            m_unhideTimer.stop();
        } else if (e->type() == QEvent::Leave) {
            m_unhideTimer.start();
        }
    }

    /*Fitt's law: if the containment has margins, and the mouse cursor clicked
     * on the mouse edge, forward the click in the containment boundaries
     */
    switch (e->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease: {
            QMouseEvent *me = static_cast<QMouseEvent *>(e);

            //first, don't mess with position if the cursor is actually outside the view:
            //somebody is doing a click and drag that must not break when the cursor i outside
            if (geometry().contains(me->screenPos().toPoint()) && !containmentContainsPosition(me->windowPos())) {
                QMouseEvent me2(me->type(),
                                positionAdjustedForContainment(me->windowPos()),
                                positionAdjustedForContainment(me->windowPos()),
                                positionAdjustedForContainment(me->windowPos()) + position(),
                                me->button(), me->buttons(), me->modifiers());

                QCoreApplication::sendEvent(this, &me2);
                return true;
            }
            break;
        }

        case QEvent::Wheel: {
            QWheelEvent *we = static_cast<QWheelEvent *>(e);

            if (!containmentContainsPosition(we->pos())) {
                QWheelEvent we2(positionAdjustedForContainment(we->pos()),
                                positionAdjustedForContainment(we->pos()) + position(),
                                we->pixelDelta(), we->angleDelta(), we->delta(),
                                we->orientation(), we->buttons(), we->modifiers(), we->phase());

                QCoreApplication::sendEvent(this, &we2);
                return true;
            }
            break;
        }

        case QEvent::DragEnter: {
            QDragEnterEvent *de = static_cast<QDragEnterEvent *>(e);
            if (!containmentContainsPosition(de->pos())) {
                QDragEnterEvent de2(positionAdjustedForContainment(de->pos()).toPoint(),
                                    de->possibleActions(), de->mimeData(), de->mouseButtons(), de->keyboardModifiers());

                QCoreApplication::sendEvent(this, &de2);
                return true;
            }
            break;
        }
        //DragLeave just works
        case QEvent::DragLeave:
            break;
        case QEvent::DragMove: {
            QDragMoveEvent *de = static_cast<QDragMoveEvent *>(e);
            if (!containmentContainsPosition(de->pos())) {
                QDragMoveEvent de2(positionAdjustedForContainment(de->pos()).toPoint(),
                                   de->possibleActions(), de->mimeData(), de->mouseButtons(), de->keyboardModifiers());

                QCoreApplication::sendEvent(this, &de2);
                return true;
            }
            break;
        }
        case QEvent::Drop: {
            QDropEvent *de = static_cast<QDropEvent *>(e);
            if (!containmentContainsPosition(de->pos())) {
                QDropEvent de2(positionAdjustedForContainment(de->pos()).toPoint(),
                               de->possibleActions(), de->mimeData(), de->mouseButtons(), de->keyboardModifiers());

                QCoreApplication::sendEvent(this, &de2);
                return true;
            }
            break;
        }

        case QEvent::Hide: {
            if (m_panelConfigView && m_panelConfigView.data()->isVisible()) {
                m_panelConfigView.data()->hide();
            }
            break;
        }
        default:
            break;
    }

    return View::event(e);
}

bool PanelView::containmentContainsPosition(const QPointF &point) const
{
    QQuickItem *containmentItem = containment()->property("_plasma_graphicObject").value<QQuickItem *>();

    if (!containmentItem) {
        return false;
    }

    return QRectF(containmentItem->mapToScene(QPoint(0,0)), QSizeF(containmentItem->width(), containmentItem->height())).contains(point);
}

QPointF PanelView::positionAdjustedForContainment(const QPointF &point) const
{
    QQuickItem *containmentItem = containment()->property("_plasma_graphicObject").value<QQuickItem *>();

    if (!containmentItem) {
        return point;
    }

    QRectF containmentRect(containmentItem->mapToScene(QPoint(0,0)), QSizeF(containmentItem->width(), containmentItem->height()));

    return QPointF(qBound(containmentRect.left() + 2, point.x(), containmentRect.right() - 2),
                   qBound(containmentRect.top() + 2, point.y(), containmentRect.bottom() - 2));
}

void PanelView::updateMask()
{
    if (KWindowSystem::compositingActive()) {
        setMask(QRegion());
    } else {
        if (!m_background) {
            m_background = new Plasma::FrameSvg(this);
            m_background->setImagePath("widgets/panel-background");
        }

        Plasma::FrameSvg::EnabledBorders borders = Plasma::FrameSvg::AllBorders;
        switch (location()) {
        case Plasma::Types::TopEdge:
            borders &= ~Plasma::FrameSvg::TopBorder;
            break;
        case Plasma::Types::LeftEdge:
            borders &= ~Plasma::FrameSvg::LeftBorder;
            break;
        case Plasma::Types::RightEdge:
            borders &= ~Plasma::FrameSvg::RightBorder;
            break;
        case Plasma::Types::BottomEdge:
            borders &= ~Plasma::FrameSvg::BottomBorder;
            break;
        default:
            break;
        }

        if (x() <= screen()->geometry().x()) {
            borders &= ~Plasma::FrameSvg::LeftBorder;
        }
        if (x() + width() >= screen()->geometry().x() + screen()->geometry().width()) {
            borders &= ~Plasma::FrameSvg::RightBorder;
        }
        if (y() <= screen()->geometry().y()) {
            borders &= ~Plasma::FrameSvg::TopBorder;
        }
        if (y() + height() >= screen()->geometry().y() + screen()->geometry().height()) {
            borders &= ~Plasma::FrameSvg::BottomBorder;
        }
        m_background->setEnabledBorders(borders);

        m_background->resizeFrame(size());
        setMask(m_background->mask());
    }
}

void PanelView::updateStruts()
{
    if (!containment() || !screen()) {
        return;
    }

    NETExtendedStrut strut;

    if (m_visibilityMode == NormalPanel) {
        const QRect thisScreen = screen()->geometry();
        const QRect wholeScreen = screen()->virtualGeometry();

        //Extended struts against a screen edge near to another screen are really harmful, so windows maximized under the panel is a lesser pain
        //TODO: force "windows can cover" in those cases?
        const int numScreens = corona()->numScreens();
        for (int i = 0; i < numScreens; ++i) {
            if (i == containment()->screen()) {
                continue;
            }

            const QRect otherScreen = corona()->screenGeometry(i);

            switch (location())
            {
                case Plasma::Types::TopEdge:
                if (otherScreen.bottom() <= thisScreen.top()) {
                    KWindowSystem::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    return;
                }
                break;
            case Plasma::Types::BottomEdge:
                if (otherScreen.top() >= thisScreen.bottom()) {
                    KWindowSystem::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    return;
                }
                break;
            case Plasma::Types::RightEdge:
                if (otherScreen.left() >= thisScreen.right()) {
                    KWindowSystem::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    return;
                }
                break;
            case Plasma::Types::LeftEdge:
                if (otherScreen.right() <= thisScreen.left()) {
                    KWindowSystem::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    return;
                }
                break;
            default:
                return;
            }
        }
        // extended struts are to the combined screen geoms, not the single screen
        int leftOffset = thisScreen.x();
        int rightOffset = wholeScreen.right() - thisScreen.right();
        int bottomOffset = wholeScreen.bottom() - thisScreen.bottom();
//         qDebug() << "screen l/r/b/t offsets are:" << leftOffset << rightOffset << bottomOffset << topOffset << location();
        int topOffset = thisScreen.top();

        switch (location())
        {
            case Plasma::Types::TopEdge:
                strut.top_width = thickness() + topOffset;
                strut.top_start = x();
                strut.top_end = x() + width() - 1;
//                 qDebug() << "setting top edge to" << strut.top_width << strut.top_start << strut.top_end;
                break;

            case Plasma::Types::BottomEdge:
                strut.bottom_width = thickness() + bottomOffset;
                strut.bottom_start = x();
                strut.bottom_end = x() + width() - 1;
//                 qDebug() << "setting bottom edge to" << strut.bottom_width << strut.bottom_start << strut.bottom_end;
                break;

            case Plasma::Types::RightEdge:
                strut.right_width = thickness() + rightOffset;
                strut.right_start = y();
                strut.right_end = y() + height() - 1;
//                 qDebug() << "setting right edge to" << strut.right_width << strut.right_start << strut.right_end;
                break;

            case Plasma::Types::LeftEdge:
                strut.left_width = thickness() + leftOffset;
                strut.left_start = y();
                strut.left_end = y() + height() - 1;
//                 qDebug() << "setting left edge to" << strut.left_width << strut.left_start << strut.left_end;
                break;

            default:
                //qDebug() << "where are we?";
                break;
        }
    }

    KWindowSystem::setExtendedStrut(winId(), strut.left_width,
                                             strut.left_start,
                                             strut.left_end,
                                             strut.right_width,
                                             strut.right_start,
                                             strut.right_end,
                                             strut.top_width,
                                             strut.top_start,
                                             strut.top_end,
                                             strut.bottom_width,
                                             strut.bottom_start,
                                             strut.bottom_end);

}

void PanelView::themeChanged()
{
    KWindowEffects::enableBlurBehind(winId(), true);
    KWindowEffects::enableBackgroundContrast(winId(), m_theme.backgroundContrastEnabled(),
                                                      m_theme.backgroundContrast(),
                                                      m_theme.backgroundIntensity(),
                                                      m_theme.backgroundSaturation());

    updateMask();
}

void PanelView::containmentChanged()
{
    positionPanel();
    connect(containment(), SIGNAL(statusChanged(Plasma::Types::ItemStatus)), SLOT(statusChanged(Plasma::Types::ItemStatus)));
    connect(containment(), &QObject::destroyed, this, [=] (QObject *obj) {
        Q_UNUSED(obj)
        //containment()->destroyed() is true only when the user deleted it
        //so the config is to be thrown away, not during shutdown
        if (containment()->destroyed()) {
            KConfigGroup views(m_corona->applicationConfig(), "PlasmaViews");
            for (auto grp : views.groupList()) {
                if (grp.contains(QRegExp("Panel " + QString::number(containment()->id()) + "$"))) {
                    qDebug() << "Panel" << containment()->id() << "removed by user";
                    views.deleteGroup(grp);
                }
                views.sync();
            }
            deleteLater();
        }
    }, Qt::QueuedConnection);
}

void PanelView::statusChanged(Plasma::Types::ItemStatus status)
{
    if (status == Plasma::Types::NeedsAttentionStatus) {
        showTemporarily();
    } else {
        restoreAutoHide();
    }
}

void PanelView::showTemporarily()
{
    setAutoHideEnabled(false);

    QTimer * t = new QTimer(this);
    t->setSingleShot(true);
    t->setInterval(3000);
    connect(t, SIGNAL(timeout()), SLOT(restoreAutoHide()));
    connect(t, SIGNAL(timeout()), t, SLOT(deleteLater()));
    t->start();
}

void PanelView::screenDestroyed(QObject* )
{
//     NOTE: this is overriding the screen destroyed slot, we need to do this because
//     otherwise Qt goes mental and starts moving our panels. See:
//     https://codereview.qt-project.org/#/c/88351/
//     if(screen == this->screen()) {
//         DO NOTHING, panels are moved by ::removeScreen
//     }
}


#include "moc_panelview.cpp"
