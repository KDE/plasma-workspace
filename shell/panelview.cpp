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
#include "screenpool.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QScreen>
#include <QQmlEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QTimer>

#include <kactioncollection.h>
#include <kwindowsystem.h>
#include <kwindoweffects.h>

#include <plasma_version.h>
#include <Plasma/Containment>
#include <Plasma/Package>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

#if HAVE_X11
#include <xcb/xcb.h>
#include <NETWM>
#include <QX11Info>
#include <QtPlatformHeaders/QXcbWindowFunctions>
#endif

static const int MINSIZE = 10;

PanelView::PanelView(ShellCorona *corona, QScreen *targetScreen, QWindow *parent)
    : PlasmaQuick::ContainmentView(corona, parent),
       m_offset(0),
       m_maxLength(0),
       m_minLength(0),
       m_contentLength(0),
       m_distance(0),
       m_thickness(30),
       m_initCompleted(false),
       m_alignment(Qt::AlignLeft),
       m_corona(corona),
       m_visibilityMode(NormalPanel),
       m_backgroundHints(Plasma::Types::StandardBackground),
       m_shellSurface(nullptr)
{
    if (targetScreen) {
        setPosition(targetScreen->geometry().center());
        setScreenToFollow(targetScreen);
        setScreen(targetScreen);
    }
    setResizeMode(QuickViewSharedEngine::SizeRootObjectToView);
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus);

    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PanelView::updateMask);
    connect(this, &PanelView::backgroundHintsChanged, this, &PanelView::updateMask);
    connect(this, &PanelView::backgroundHintsChanged, this, &PanelView::updateEnabledBorders);
    // TODO: add finished/componentComplete signal to QuickViewSharedEngine,
    // so we exactly know when rootobject is available
    connect(this, &QuickViewSharedEngine::statusChanged,
            this, &PanelView::handleQmlStatusChange);

    m_positionPaneltimer.setSingleShot(true);
    m_positionPaneltimer.setInterval(150);
    connect(&m_positionPaneltimer, &QTimer::timeout, this, &PanelView::restore);

    m_unhideTimer.setSingleShot(true);
    m_unhideTimer.setInterval(500);
    connect(&m_unhideTimer, &QTimer::timeout,
            this, &PanelView::restoreAutoHide);

    m_lastScreen = targetScreen;
    connect(this, SIGNAL(locationChanged(Plasma::Types::Location)),
            &m_positionPaneltimer, SLOT(start()));
    connect(this, SIGNAL(containmentChanged()),
            this, SLOT(containmentChanged()));

    if (!m_corona->kPackage().isValid()) {
        qWarning() << "Invalid home screen package";
    }

    m_strutsTimer.setSingleShot(true);
    connect(&m_strutsTimer, &QTimer::timeout,
            this, &PanelView::updateStruts);

    qmlRegisterType<QScreen>();
    rootContext()->setContextProperty(QStringLiteral("panel"), this);
    setSource(m_corona->kPackage().fileUrl("views", QStringLiteral("Panel.qml")));
}

PanelView::~PanelView()
{
    if (containment()) {
        m_corona->requestApplicationConfigSync();
    }
}

KConfigGroup PanelView::panelConfig(ShellCorona *corona, Plasma::Containment *containment, QScreen *screen)
{
    if (!containment || !screen) {
        return KConfigGroup();
    }
    KConfigGroup views(corona->applicationConfig(), "PlasmaViews");
    views = KConfigGroup(&views, QStringLiteral("Panel %1").arg(containment->id()));

    if (containment->formFactor() == Plasma::Types::Vertical) {
        return KConfigGroup(&views, QStringLiteral("Vertical") + QString::number(screen->size().height()));
    //treat everything else as horizontal
    } else {
        return KConfigGroup(&views, QStringLiteral("Horizontal") + QString::number(screen->size().width()));
    }
}

KConfigGroup PanelView::panelConfigDefaults(ShellCorona *corona, Plasma::Containment *containment, QScreen *screen)
{
    if (!containment || !screen) {
        return KConfigGroup();
    }

    KConfigGroup views(corona->applicationConfig(), "PlasmaViews");
    views = KConfigGroup(&views, QStringLiteral("Panel %1").arg(containment->id()));

    return KConfigGroup(&views, QStringLiteral("Defaults"));
}

int PanelView::readConfigValueWithFallBack(const QString &key, int defaultValue)
{
    int value = config().readEntry(key, configDefaults().readEntry(key, defaultValue));
    return value;
}

KConfigGroup PanelView::config() const
{
    return panelConfig(m_corona, containment(), m_screenToFollow);
}

KConfigGroup PanelView::configDefaults() const
{
    return panelConfigDefaults(m_corona, containment(), m_screenToFollow);
}

Q_INVOKABLE QString PanelView::fileFromPackage(const QString &key, const QString &fileName)
{
    return corona()->kPackage().filePath(key.toUtf8(), fileName);
}

void PanelView::maximize()
{
    int length;
    if (containment()->formFactor() == Plasma::Types::Vertical) {
        length = m_screenToFollow->size().height();
    } else {
        length = m_screenToFollow->size().width();
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
    //alignment is not resolution dependent, doesn't save to Defaults
    config().parent().writeEntry("alignment", (int)m_alignment);
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
        if (offset + m_maxLength > m_screenToFollow->size().height()) {
            setMaximumLength( -m_offset + m_screenToFollow->size().height() );
        }
    } else {
        if (offset + m_maxLength > m_screenToFollow->size().width()) {
            setMaximumLength( -m_offset + m_screenToFollow->size().width() );
        }
    }

    m_offset = offset;
    config().writeEntry("offset", m_offset);
    configDefaults().writeEntry("offset", m_offset);
    positionPanel();
    emit offsetChanged();
    m_corona->requestApplicationConfigSync();
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
    emit thicknessChanged();

    config().writeEntry("thickness", value);
    configDefaults().writeEntry("thickness", value);
    m_corona->requestApplicationConfigSync();
    resizePanel();
}

int PanelView::length() const
{
    return qMax(1, m_contentLength);
}

void PanelView::setLength(int value)
{
    if (value == m_contentLength) {
        return;
    }

    m_contentLength = value;

    resizePanel();
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

    config().writeEntry("maxLength", length);
    configDefaults().writeEntry("maxLength", length);
    m_maxLength = length;
    emit maximumLengthChanged();
    m_corona->requestApplicationConfigSync();

    resizePanel();
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

    config().writeEntry("minLength", length);
    configDefaults().writeEntry("minLength", length);
    m_minLength = length;
    emit minimumLengthChanged();
    m_corona->requestApplicationConfigSync();

    resizePanel();
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

Plasma::Types::BackgroundHints PanelView::backgroundHints() const
{
    return m_backgroundHints;
}

void PanelView::setBackgroundHints(Plasma::Types::BackgroundHints hint)
{
    if (m_backgroundHints == hint) {
       return;
    }
 
    m_backgroundHints = hint;
     
    emit backgroundHintsChanged();
}

Plasma::FrameSvg::EnabledBorders PanelView::enabledBorders() const
{
    return m_enabledBorders;
}

void PanelView::setVisibilityMode(PanelView::VisibilityMode mode)
{
    if (m_visibilityMode == mode) {
        return;
    }

    m_visibilityMode = mode;

    disconnect(containment(), &Plasma::Applet::activated, this, &PanelView::showTemporarily);
    if (edgeActivated()) {
        connect(containment(), &Plasma::Applet::activated, this, &PanelView::showTemporarily);
    }

    if (config().isValid() && config().parent().isValid()) {
        //panelVisibility is not resolution dependent, don't write to Defaults
        config().parent().writeEntry("panelVisibility", (int)mode);
        m_corona->requestApplicationConfigSync();
    }

    visibilityModeToWayland();
    updateStruts();

    emit visibilityModeChanged();

    restoreAutoHide();
}

void PanelView::visibilityModeToWayland()
{
    if (!m_shellSurface) {
        return;
    }
    KWayland::Client::PlasmaShellSurface::PanelBehavior behavior;
    switch (m_visibilityMode) {
    case NormalPanel:
        behavior = KWayland::Client::PlasmaShellSurface::PanelBehavior::AlwaysVisible;
        break;
    case AutoHide:
        behavior = KWayland::Client::PlasmaShellSurface::PanelBehavior::AutoHide;
        break;
    case LetWindowsCover:
        behavior = KWayland::Client::PlasmaShellSurface::PanelBehavior::WindowsCanCover;
        break;
    case WindowsGoBelow:
        behavior = KWayland::Client::PlasmaShellSurface::PanelBehavior::WindowsGoBelow;
        break;
    default:
        Q_UNREACHABLE();
        return;
    }
    m_shellSurface->setPanelBehavior(behavior);
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

    if (!m_initCompleted) {
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
    const QPoint pos = geometryByDistance(m_distance).topLeft();
    setPosition(pos);

    if (m_shellSurface) {
        m_shellSurface->setPosition(pos);
    }

    KWindowEffects::slideWindow(winId(), slideLocation, -1);
}

QRect PanelView::geometryByDistance(int distance) const
{
    QScreen *s = m_screenToFollow;
    QPoint position;
    const QRect screenGeometry = s->geometry();

    switch (containment()->location()) {
    case Plasma::Types::TopEdge:
        switch (m_alignment) {
        case Qt::AlignCenter:
            position = QPoint(QPoint(screenGeometry.center().x(), screenGeometry.top()) + QPoint(m_offset - width()/2, distance));
            break;
        case Qt::AlignRight:
            position = QPoint(QPoint(screenGeometry.x() + screenGeometry.width(), screenGeometry.y()) - QPoint(m_offset + width(), distance));
            break;
        case Qt::AlignLeft:
        default:
            position = QPoint(screenGeometry.topLeft() + QPoint(m_offset, distance));
        }
        break;

    case Plasma::Types::LeftEdge:
        switch (m_alignment) {
        case Qt::AlignCenter:
            position = QPoint(QPoint(screenGeometry.left(), screenGeometry.center().y()) + QPoint(distance, m_offset - height()/2));
            break;
        case Qt::AlignRight:
            position = QPoint(QPoint(screenGeometry.left(), screenGeometry.y() + screenGeometry.height()) - QPoint(distance, m_offset + height()));
            break;
        case Qt::AlignLeft:
        default:
            position = QPoint(screenGeometry.topLeft() + QPoint(distance, m_offset));
        }
        break;

    case Plasma::Types::RightEdge:
        switch (m_alignment) {
        case Qt::AlignCenter:
            // Never use rect.right(); for historical reasons it returns left() + width() - 1; see https://doc.qt.io/qt-5/qrect.html#right
            position = QPoint(QPoint(screenGeometry.x() + screenGeometry.width(), screenGeometry.center().y()) - QPoint(thickness() + distance, 0) + QPoint(0, m_offset - height()/2));
            break;
        case Qt::AlignRight:
            position = QPoint(QPoint(screenGeometry.x() + screenGeometry.width(), screenGeometry.y() + screenGeometry.height()) - QPoint(thickness() + distance, 0) - QPoint(0, m_offset + height()));
            break;
        case Qt::AlignLeft:
        default:
            position = QPoint(QPoint(screenGeometry.x() + screenGeometry.width(), screenGeometry.y()) - QPoint(thickness() + distance, 0) + QPoint(0, m_offset));
        }
        break;

    case Plasma::Types::BottomEdge:
    default:
        switch (m_alignment) {
        case Qt::AlignCenter:
            position = QPoint(QPoint(screenGeometry.center().x(), screenGeometry.bottom() - thickness() - distance) + QPoint(m_offset - width()/2, 1));
            break;
        case Qt::AlignRight:
            position = QPoint(screenGeometry.bottomRight() - QPoint(0, thickness() + distance) - QPoint(m_offset + width(), -1));
            break;
        case Qt::AlignLeft:
        default:
            position = QPoint(screenGeometry.bottomLeft() - QPoint(0, thickness() + distance) + QPoint(m_offset, 1));
        }
    }
    QRect ret = formFactor() == Plasma::Types::Vertical ? QRect(position, QSize(thickness(), height())) : QRect(position, QSize(width(), thickness()));
    ret = ret.intersected(screenGeometry);
    return ret;
}

void PanelView::resizePanel()
{
    if (!m_initCompleted) {
        return;
    }

    QSize targetSize;
    QSize targetMinSize;
    QSize targetMaxSize;

    if (formFactor() == Plasma::Types::Vertical) {
        const int minSize = qMax(MINSIZE, m_minLength);
        const int maxSize = qMin(m_maxLength, m_screenToFollow->size().height() - m_offset);
        targetMinSize = QSize(thickness(), minSize);
        targetMaxSize = QSize(thickness(), maxSize);
        targetSize = QSize(thickness(), qBound(minSize, m_contentLength, maxSize));
    } else {
        const int minSize = qMax(MINSIZE, m_minLength);
        const int maxSize = qMin(m_maxLength, m_screenToFollow->size().width() - m_offset);
        targetMinSize = QSize(minSize, thickness());
        targetMaxSize = QSize(maxSize, thickness());
        targetSize = QSize(qBound(minSize, m_contentLength, maxSize), thickness());
    }
    if (minimumSize() != targetMinSize) {
        setMinimumSize(targetMinSize);
    }
    if (maximumSize() != targetMaxSize) {
        setMaximumSize(targetMaxSize);
    }
    if (size() != targetSize) {
        resize(targetSize);
    }

    //position will be updated implicitly from resizeEvent
}

void PanelView::restore()
{
    if (!containment()) {
        return;
    }

    // All the defaults are based on whatever are the current values
    // so won't be weirdly reset after screen resolution change

    //alignment is not resolution dependent
    //but if fails read it from the resolution dependent one as
    //the place for this config key is changed in Plasma 5.9
    //Do NOT use readConfigValueWithFallBack
    setAlignment((Qt::Alignment)config().parent().readEntry<int>("alignment", config().readEntry<int>("alignment", m_alignment)));

    // All the other values are read from screen independent values,
    // but fallback on the screen independent section, as is the only place
    // is safe to directly write during plasma startup, as there can be 
    // resolution changes
    m_offset = readConfigValueWithFallBack("offset", m_offset);
    if (m_alignment != Qt::AlignCenter) {
        m_offset = qMax(0, m_offset);
    }

    setThickness(readConfigValueWithFallBack("thickness", m_thickness));

    const QSize screenSize = m_screenToFollow->size();
    setMinimumSize(QSize(-1, -1));
    //FIXME: an invalid size doesn't work with QWindows
    setMaximumSize(screenSize);

    const int side = containment()->formFactor() == Plasma::Types::Vertical ? screenSize.height() : screenSize.width();
    const int maxSize = side - m_offset;
    m_maxLength = qBound<int>(MINSIZE, readConfigValueWithFallBack("maxLength", side), maxSize);
    m_minLength = qBound<int>(MINSIZE, readConfigValueWithFallBack("minLength", side), maxSize);

    //panelVisibility is not resolution dependent
    //but if fails read it from the resolution dependent one as
    //the place for this config key is changed in Plasma 5.9
    //Do NOT use readConfigValueWithFallBack
    setVisibilityMode((VisibilityMode)config().parent().readEntry<int>("panelVisibility", config().readEntry<int>("panelVisibility", (int)NormalPanel)));
    m_initCompleted = true;
    resizePanel();
    positionPanel();

    emit maximumLengthChanged();
    emit minimumLengthChanged();
    emit offsetChanged();
    emit alignmentChanged();

    //::restore might have been called directly before the timer fires
    // at which point we don't still need the timer
    m_positionPaneltimer.stop();
}

void PanelView::showConfigurationInterface(Plasma::Applet *applet)
{
    if (!applet || !applet->containment()) {
        return;
    }

    Plasma::Containment *cont = qobject_cast<Plasma::Containment *>(applet);

    const bool isPanelConfig = (cont && cont == containment() && cont->isContainment());

    if (m_panelConfigView) {
        if (m_panelConfigView->applet() == applet) {
            if (isPanelConfig) { // toggles panel controller, whereas applet config is always brought to front
                if (m_panelConfigView->isVisible()) {
                    m_panelConfigView->hide();
                } else {
                    m_panelConfigView->show();
                }
                return;
            }

            m_panelConfigView->show();
            m_panelConfigView->requestActivate();
            return;
        }

        m_panelConfigView->hide();
        m_panelConfigView->deleteLater();
    }

    if (isPanelConfig) {
        m_panelConfigView = new PanelConfigView(cont, this);
    } else {
        m_panelConfigView = new PlasmaQuick::ConfigView(applet);
    }

    m_panelConfigView.data()->init();
    m_panelConfigView.data()->show();
    m_panelConfigView.data()->requestActivate();

    if (isPanelConfig) {
        KWindowSystem::setState(m_panelConfigView.data()->winId(), NET::SkipTaskbar | NET::SkipPager);
    }
}

void PanelView::restoreAutoHide()
{
    bool autoHide = true;
    disconnect(m_transientWindowVisibleWatcher);

    if (!edgeActivated()) {
        autoHide = false;
    }
    else if (m_containsMouse) {
        autoHide = false;
    }
    else if (containment() && containment()->isUserConfiguring()) {
        autoHide = false;
    }
    else if (containment() && containment()->status() >= Plasma::Types::NeedsAttentionStatus &&
             containment()->status() != Plasma::Types::HiddenStatus) {
        autoHide = false;
    } else {
        for(QWindow *window: qApp->topLevelWindows()) {
            if (window->transientParent() == this && window->isVisible()) {
                m_transientWindowVisibleWatcher = connect(window, &QWindow::visibleChanged, this, &PanelView::restoreAutoHide);
                autoHide = false;
                break; //if multiple are open, we will re-evaluate this expression after the first closes
            }
        }
    }

    setAutoHideEnabled(autoHide);
}

void PanelView::setAutoHideEnabled(bool enabled)
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        xcb_connection_t *c = QX11Info::connection();

        const QByteArray effectName = QByteArrayLiteral("_KDE_NET_WM_SCREEN_EDGE_SHOW");
        xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom_unchecked(c, false, effectName.length(), effectName.constData());

        QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> atom(xcb_intern_atom_reply(c, atomCookie, nullptr));

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

        int hideType = 0;
        if (m_visibilityMode == LetWindowsCover) {
            hideType = 1;
        }
        value |= hideType << 8;

        xcb_change_property(c, XCB_PROP_MODE_REPLACE, winId(), atom->atom, XCB_ATOM_CARDINAL, 32, 1, &value);
        KWindowEffects::slideWindow(winId(), slideLocation, -1);
    }
#endif
    if (m_shellSurface && (m_visibilityMode == PanelView::AutoHide || m_visibilityMode == PanelView::LetWindowsCover)) {
        if (enabled) {
            m_shellSurface->requestHideAutoHidingPanel();
        } else {
            if (m_visibilityMode == PanelView::AutoHide)
                m_shellSurface->requestShowAutoHidingPanel();
        }
    }
}

void PanelView::resizeEvent(QResizeEvent *ev)
{
    updateEnabledBorders();
    //don't setGeometry() to make really sure we aren't doing a resize loop
    const QPoint pos = geometryByDistance(m_distance).topLeft();
    setPosition(pos);
    if (m_shellSurface) {
        m_shellSurface->setPosition(pos);
    }
    m_strutsTimer.start(STRUTSTIMERDELAY);
    emit m_corona->availableScreenRegionChanged();

    PlasmaQuick::ContainmentView::resizeEvent(ev);

#if PLASMA_VERSION < QT_VERSION_CHECK(5,59,0)
    updateMask();
#endif
}

void PanelView::moveEvent(QMoveEvent *ev)
{
    updateEnabledBorders();
    m_strutsTimer.start(STRUTSTIMERDELAY);
    PlasmaQuick::ContainmentView::moveEvent(ev);
#if PLASMA_VERSION < QT_VERSION_CHECK(5,59,0)
    updateMask();
#endif
}

void PanelView::integrateScreen()
{
    connect(m_screenToFollow.data(), &QScreen::geometryChanged, this, &PanelView::restore);
    updateMask();
    KWindowSystem::setOnAllDesktops(winId(), true);
    KWindowSystem::setType(winId(), NET::Dock);
#if HAVE_X11
    QXcbWindowFunctions::setWmWindowType(this, QXcbWindowFunctions::Dock);
#endif
    if (m_shellSurface) {
        m_shellSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Panel);
        m_shellSurface->setSkipTaskbar(true);
    }
    setVisibilityMode(m_visibilityMode);

    if (containment()) {
        containment()->reactToScreenChange();
    }
}

void PanelView::showEvent(QShowEvent *event)
{
    PlasmaQuick::ContainmentView::showEvent(event);

    integrateScreen();
}

void PanelView::setScreenToFollow(QScreen *screen)
{
    if (screen == m_screenToFollow) {
        return;
    }

    if (!screen) {
        return;
    }

    /*connect(screen, &QObject::destroyed, this, [this]() {
        if (PanelView::screen()) {
            m_screenToFollow = PanelView::screen();
            adaptToScreen();
        }
    });*/

    m_screenToFollow = screen;
    setScreen(screen);
    adaptToScreen();
}

QScreen *PanelView::screenToFollow() const
{
    return m_screenToFollow;
}

void PanelView::adaptToScreen()
{
    emit screenToFollowChanged(m_screenToFollow);
    m_lastScreen = m_screenToFollow;

    if (!m_screenToFollow) {
        return;
    }

    integrateScreen();
    showTemporarily();
    m_positionPaneltimer.start();
}

bool PanelView::event(QEvent *e)
{
    switch (e->type()) {
        case QEvent::Enter:
            m_containsMouse = true;
            if (edgeActivated()) {
                m_unhideTimer.stop();
            }
            break;

        case QEvent::Leave:
            m_containsMouse = false;
            if (edgeActivated()) {
                m_unhideTimer.start();
            }
            break;

        /*Fitt's law: if the containment has margins, and the mouse cursor clicked
        * on the mouse edge, forward the click in the containment boundaries
        */

        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease: {
            QMouseEvent *me = static_cast<QMouseEvent *>(e);

            //first, don't mess with position if the cursor is actually outside the view:
            //somebody is doing a click and drag that must not break when the cursor i outside
            if (geometry().contains(QCursor::pos(screenToFollow()))) {
                if (!containmentContainsPosition(me->windowPos())) {
                    auto me2 = new QMouseEvent(me->type(),
                                    positionAdjustedForContainment(me->windowPos()),
                                    positionAdjustedForContainment(me->windowPos()),
                                    positionAdjustedForContainment(me->windowPos()) + position(),
                                    me->button(), me->buttons(), me->modifiers());

                    QCoreApplication::postEvent(this, me2);
                    return true;
                }
            } else {
                // default handling if current mouse position is outside the panel
                return ContainmentView::event(e);
            }
            break;
        }

        case QEvent::Wheel: {
            QWheelEvent *we = static_cast<QWheelEvent *>(e);

            if (!containmentContainsPosition(we->pos())) {
                auto we2 = new QWheelEvent(positionAdjustedForContainment(we->pos()),
                                positionAdjustedForContainment(we->pos()) + position(),
                                we->pixelDelta(), we->angleDelta(), we->angleDelta().y(),
                                we->orientation(), we->buttons(), we->modifiers(), we->phase());

                QCoreApplication::postEvent(this, we2);
                return true;
            }
            break;
        }

        case QEvent::DragEnter: {
            QDragEnterEvent *de = static_cast<QDragEnterEvent *>(e);
            if (!containmentContainsPosition(de->pos())) {
                auto de2 = new QDragEnterEvent(positionAdjustedForContainment(de->pos()).toPoint(),
                                    de->possibleActions(), de->mimeData(), de->mouseButtons(), de->keyboardModifiers());

                QCoreApplication::postEvent(this, de2);
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
                auto de2 = new QDragMoveEvent(positionAdjustedForContainment(de->pos()).toPoint(),
                                   de->possibleActions(), de->mimeData(), de->mouseButtons(), de->keyboardModifiers());

                QCoreApplication::postEvent(this, de2);
                return true;
            }
            break;
        }
        case QEvent::Drop: {
            QDropEvent *de = static_cast<QDropEvent *>(e);
            if (!containmentContainsPosition(de->pos())) {
                auto de2 = new QDropEvent(positionAdjustedForContainment(de->pos()).toPoint(),
                               de->possibleActions(), de->mimeData(), de->mouseButtons(), de->keyboardModifiers());

                QCoreApplication::postEvent(this, de2);
                return true;
            }
            break;
        }

        case QEvent::Hide: {
            if (m_panelConfigView && m_panelConfigView.data()->isVisible()) {
                m_panelConfigView.data()->hide();
            }
            m_containsMouse = false;
            break;
        }
        case QEvent::PlatformSurface:
            switch (static_cast<QPlatformSurfaceEvent*>(e)->surfaceEventType()) {
            case QPlatformSurfaceEvent::SurfaceCreated:
                setupWaylandIntegration();
                PanelShadows::self()->addWindow(this, enabledBorders());
                break;
            case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
                delete m_shellSurface;
                m_shellSurface = nullptr;
                PanelShadows::self()->removeWindow(this);
                break;
            }
            break;
        default:
            break;
    }

    return ContainmentView::event(e);
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
    if (m_backgroundHints == Plasma::Types::NoBackground) {
        KWindowEffects::enableBlurBehind(winId(), false);
        KWindowEffects::enableBackgroundContrast(winId(), false);
        setMask(QRegion());
    } else {
        QRegion mask;

        QQuickItem *rootObject = this->rootObject();
        if (rootObject) {
            const QVariant maskProperty = rootObject->property("panelMask");
            if (static_cast<QMetaType::Type>(maskProperty.type()) == QMetaType::QRegion) {
                mask = maskProperty.value<QRegion>();
            }
        }

        KWindowEffects::enableBlurBehind(winId(), m_theme.blurBehindEnabled(), mask);
        KWindowEffects::enableBackgroundContrast(winId(), m_theme.backgroundContrastEnabled(),
                                                          m_theme.backgroundContrast(),
                                                          m_theme.backgroundIntensity(),
                                                          m_theme.backgroundSaturation(),
                                                          mask);

        if (KWindowSystem::compositingActive()) {
            setMask(QRegion());
        } else {
            setMask(mask);
        }
    }
}

bool PanelView::canSetStrut() const
{
#if HAVE_X11
    if (!KWindowSystem::isPlatformX11()) {
        return true;
    }
    // read the wm name, need to do this every time which means a roundtrip unfortunately
    // but WM might have changed
    NETRootInfo rootInfo(QX11Info::connection(), NET::Supported | NET::SupportingWMCheck);
    if (qstricmp(rootInfo.wmName(), "KWin") == 0) {
        // KWin since 5.7 can handle this fine, so only exclude for other window managers
        return true;
    }

    const QRect thisScreen = screen()->geometry();
    const int numScreens = corona()->numScreens();
    if (numScreens < 2) {
        return true;
    }

    //Extended struts against a screen edge near to another screen are really harmful, so windows maximized under the panel is a lesser pain
    //TODO: force "windows can cover" in those cases?
    const auto screenIds = m_corona->screenIds();
    for (int id : screenIds) {
        if (id == containment()->screen()) {
            continue;
        }

        const QRect otherScreen = corona()->screenGeometry(id);
        if (!otherScreen.isValid()) {
            continue;
        }

        switch (location())
        {
            case Plasma::Types::TopEdge:
            if (otherScreen.bottom() <= thisScreen.top()) {
                return false;
            }
            break;
        case Plasma::Types::BottomEdge:
            if (otherScreen.top() >= thisScreen.bottom()) {
                return false;
            }
            break;
        case Plasma::Types::RightEdge:
            if (otherScreen.left() >= thisScreen.right()) {
                return false;
            }
            break;
        case Plasma::Types::LeftEdge:
            if (otherScreen.right() <= thisScreen.left()) {
                return false;
            }
            break;
        default:
            return false;
        }
    }
    return true;
#else
    return true;
#endif
}

void PanelView::updateStruts()
{
    if (!containment() || containment()->isUserConfiguring() || !m_screenToFollow) {
        return;
    }

    NETExtendedStrut strut;

    if (m_visibilityMode == NormalPanel) {
        const QRect thisScreen = m_screenToFollow->geometry();
        // QScreen::virtualGeometry() is very unreliable (Qt 5.5)
        const QRect wholeScreen = QRect(QPoint(0, 0), m_screenToFollow->virtualSize());

        if (!canSetStrut()) {
            KWindowSystem::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            return;
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

void PanelView::containmentChanged()
{
    restore();
    connect(containment(), &Plasma::Containment::userConfiguringChanged, this, [this](bool configuring){
        if (configuring) {
            showTemporarily();
        } else {
            m_unhideTimer.start();
            updateStruts();
        }
    });

    connect(containment(), SIGNAL(statusChanged(Plasma::Types::ItemStatus)), SLOT(statusChanged(Plasma::Types::ItemStatus)));
    connect(containment(), &Plasma::Applet::appletDeleted, this, [this] {
        //containment()->destroyed() is true only when the user deleted it
        //so the config is to be thrown away, not during shutdown
        if (containment()->destroyed()) {
            KConfigGroup views(m_corona->applicationConfig(), "PlasmaViews");
            for (auto grp : views.groupList()) {
                if (grp.contains(QRegularExpression(QStringLiteral("Panel %1$").arg(
                                                        QString::number(containment()->id()))))) {
                    qDebug() << "Panel" << containment()->id() << "removed by user";
                    views.deleteGroup(grp);
                }
                views.sync();
            }
        }
    });
}

void PanelView::handleQmlStatusChange(QQmlComponent::Status status)
{
    if (status != QQmlComponent::Ready) {
        return;
    }

    QQuickItem *rootObject = this->rootObject();
    if (rootObject) {
        disconnect(this, &QuickViewSharedEngine::statusChanged,
                   this, &PanelView::handleQmlStatusChange);

        const QVariant maskProperty = rootObject->property("panelMask");
        if (static_cast<QMetaType::Type>(maskProperty.type()) == QMetaType::QRegion) {
            connect(rootObject, SIGNAL(panelMaskChanged()),
                    this, SLOT(updateMask()));
            updateMask();
        }
    }
}

void PanelView::statusChanged(Plasma::Types::ItemStatus status)
{
    if (status == Plasma::Types::NeedsAttentionStatus) {
        showTemporarily();
        setFlags(flags() | Qt::WindowDoesNotAcceptFocus);
    } else if (status == Plasma::Types::AcceptingInputStatus) {
        setFlags(flags() & ~Qt::WindowDoesNotAcceptFocus);
        KWindowSystem::forceActiveWindow(winId());
    } else {
        restoreAutoHide();
        setFlags(flags() | Qt::WindowDoesNotAcceptFocus);
    }
}

void PanelView::showTemporarily()
{
    setAutoHideEnabled(false);

    QTimer * t = new QTimer(this);
    t->setSingleShot(true);
    t->setInterval(3000);
    connect(t, &QTimer::timeout, this, &PanelView::restoreAutoHide);
    connect(t, &QTimer::timeout, t, &QObject::deleteLater);
    t->start();
}

void PanelView::screenDestroyed(QObject* )
{
//     NOTE: this is overriding the screen destroyed slot, we need to do this because
//     otherwise Qt goes mental and starts moving our panels. See:
//     https://codereview.qt-project.org/#/c/88351/
//     if(screen == this->m_screenToFollow) {
//         DO NOTHING, panels are moved by ::readaptToScreen
//     }
}

void PanelView::setupWaylandIntegration()
{
    if (m_shellSurface) {
        // already setup
        return;
    }
    if (ShellCorona *c = qobject_cast<ShellCorona*>(corona())) {
        using namespace KWayland::Client;
        PlasmaShell *interface = c->waylandPlasmaShellInterface();
        if (!interface) {
            return;
        }
        Surface *s = Surface::fromWindow(this);
        if (!s) {
            return;
        }
        m_shellSurface = interface->createSurface(s, this);
    }
}

bool PanelView::edgeActivated() const
{
    return m_visibilityMode == PanelView::AutoHide || m_visibilityMode == LetWindowsCover;
}

void PanelView::updateEnabledBorders()
{
    Plasma::FrameSvg::EnabledBorders borders = Plasma::FrameSvg::AllBorders;

    if (m_backgroundHints == Plasma::Types::NoBackground) {
        borders = Plasma::FrameSvg::NoBorder;
    } else {
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

        if (x() <= m_screenToFollow->geometry().x()) {
            borders &= ~Plasma::FrameSvg::LeftBorder;
        }
        if (x() + width() >= m_screenToFollow->geometry().x() + m_screenToFollow->geometry().width()) {
            borders &= ~Plasma::FrameSvg::RightBorder;
        }
        if (y() <= m_screenToFollow->geometry().y()) {
            borders &= ~Plasma::FrameSvg::TopBorder;
        }
        if (y() + height() >= m_screenToFollow->geometry().y() + m_screenToFollow->geometry().height()) {
            borders &= ~Plasma::FrameSvg::BottomBorder;
        }
    }

    if (m_enabledBorders != borders) {
        PanelShadows::self()->setEnabledBorders(this, borders);
        m_enabledBorders = borders;
        emit enabledBordersChanged();
    }
}


#include "moc_panelview.cpp"
