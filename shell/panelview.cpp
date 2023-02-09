/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-plasma.h>

#include "../c_ptr.h"
#include "debug.h"
#include "panelconfigview.h"
#include "panelshadows_p.h"
#include "panelview.h"
#include "screenpool.h"
#include "shellcorona.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QScreen>

#include <KX11Extras>
#include <kactioncollection.h>
#include <kwindoweffects.h>
#include <kwindowsystem.h>

#include <Plasma/Containment>
#include <Plasma/Package>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

#if HAVE_X11
#include <NETWM>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#include <qpa/qplatformwindow_p.h>
#else
#include <QX11Info>
#include <QtPlatformHeaders/QXcbWindowFunctions>
#endif
#include <xcb/xcb.h>
#endif
#include <chrono>

using namespace std::chrono_literals;
static const int MINSIZE = 10;

PanelView::PanelView(ShellCorona *corona, QScreen *targetScreen, QWindow *parent)
    : PlasmaQuick::ContainmentView(corona, parent)
    , m_offset(0)
    , m_maxLength(0)
    , m_minLength(0)
    , m_contentLength(0)
    , m_distance(0)
    , m_thickness(30)
    , m_minDrawingWidth(0)
    , m_minDrawingHeight(0)
    , m_initCompleted(false)
    , m_floating(false)
    , m_alignment(Qt::AlignLeft)
    , m_corona(corona)
    , m_visibilityMode(NormalPanel)
    , m_opacityMode(Adaptive)
    , m_backgroundHints(Plasma::Types::StandardBackground)
    , m_shellSurface(nullptr)
{
    if (targetScreen) {
        setPosition(targetScreen->geometry().center());
        setScreenToFollow(targetScreen);
        setScreen(targetScreen);
    }
    setResizeMode(QuickViewSharedEngine::SizeRootObjectToView);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    updateAdaptiveOpacityEnabled();

    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PanelView::updateMask);
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PanelView::updateAdaptiveOpacityEnabled);
    connect(this, &PanelView::backgroundHintsChanged, this, &PanelView::updateMask);
    connect(this, &PanelView::backgroundHintsChanged, this, &PanelView::updateEnabledBorders);
    // TODO: add finished/componentComplete signal to QuickViewSharedEngine,
    // so we exactly know when rootobject is available
    connect(this, &QuickViewSharedEngine::statusChanged, this, &PanelView::handleQmlStatusChange);

    m_unhideTimer.setSingleShot(true);
    m_unhideTimer.setInterval(500ms);
    connect(&m_unhideTimer, &QTimer::timeout, this, &PanelView::restoreAutoHide);

    m_lastScreen = targetScreen;
    connect(this, &PanelView::locationChanged, this, &PanelView::restore);
    connect(this, &PanelView::containmentChanged, this, &PanelView::refreshContainment);

    // FEATURE 352476: cancel focus on the panel when clicking outside
    connect(this, &PanelView::activeFocusItemChanged, this, [this] {
        if (containment()->status() == Plasma::Types::AcceptingInputStatus && !activeFocusItem()) {
            // BUG 454729: avoid switching to PassiveStatus in keyboard navigation
            containment()->setStatus(Plasma::Types::ActiveStatus);
        }
    });

    if (!m_corona->kPackage().isValid()) {
        qCWarning(PLASMASHELL) << "Invalid home screen package";
    }

    m_strutsTimer.setSingleShot(true);
    connect(&m_strutsTimer, &QTimer::timeout, this, &PanelView::updateStruts);

    // Register enums
    qmlRegisterUncreatableMetaObject(PanelView::staticMetaObject, "org.kde.plasma.shell.panel", 0, 1, "Global", QStringLiteral("Error: only enums"));

    qmlRegisterAnonymousType<QScreen>("", 1);
    rootContext()->setContextProperty(QStringLiteral("panel"), this);
    setSource(m_corona->kPackage().fileUrl("views", QStringLiteral("Panel.qml")));
    updatePadding();
    updateFloating();
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
        // treat everything else as horizontal
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
    // alignment is not resolution dependent, doesn't save to Defaults
    config().parent().writeEntry("alignment", (int)m_alignment);
    Q_EMIT alignmentChanged();
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
            setMaximumLength(-m_offset + m_screenToFollow->size().height());
        }
    } else {
        if (offset + m_maxLength > m_screenToFollow->size().width()) {
            setMaximumLength(-m_offset + m_screenToFollow->size().width());
        }
    }

    m_offset = offset;
    config().writeEntry("offset", m_offset);
    configDefaults().writeEntry("offset", m_offset);
    positionPanel();
    Q_EMIT offsetChanged();
    m_corona->requestApplicationConfigSync();
    Q_EMIT m_corona->availableScreenRegionChanged();
}

int PanelView::thickness() const
{
    return m_thickness;
}

int PanelView::totalThickness() const
{
    switch (containment()->location()) {
    case Plasma::Types::TopEdge:
        return thickness() + m_topFloatingPadding + m_bottomFloatingPadding;
    case Plasma::Types::LeftEdge:
        return thickness() + m_leftFloatingPadding + m_rightFloatingPadding;
    case Plasma::Types::RightEdge:
        return thickness() + m_rightFloatingPadding + m_leftFloatingPadding;
    case Plasma::Types::BottomEdge:
        return thickness() + m_bottomFloatingPadding + m_topFloatingPadding;
    default:
        return thickness();
    }
}

void PanelView::setThickness(int value)
{
    if (value < minThickness()) {
        value = minThickness();
    }
    if (value == thickness()) {
        return;
    }

    m_thickness = value;
    Q_EMIT thicknessChanged();

    // For thickness, always use the default thickness value for horizontal/vertical panel
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
    Q_EMIT maximumLengthChanged();
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
    Q_EMIT minimumLengthChanged();
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
    Q_EMIT distanceChanged();
    positionPanel();
}

bool PanelView::floating() const
{
    return m_floating;
}

void PanelView::setFloating(bool floating)
{
    if (m_floating == floating) {
        return;
    }

    m_floating = floating;
    if (config().isValid() && config().parent().isValid()) {
        config().parent().writeEntry("floating", (int)floating);
        m_corona->requestApplicationConfigSync();
    }
    Q_EMIT floatingChanged();

    updateFloating();
    updateEnabledBorders();
    updateMask();
}

int PanelView::minThickness() const
{
    if (formFactor() == Plasma::Types::Vertical) {
        return m_minDrawingWidth;
    } else if (formFactor() == Plasma::Types::Horizontal) {
        return m_minDrawingHeight;
    }
    return 0;
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

    Q_EMIT backgroundHintsChanged();
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
        // panelVisibility is not resolution dependent, don't write to Defaults
        config().parent().writeEntry("panelVisibility", (int)mode);
        m_corona->requestApplicationConfigSync();
    }

    visibilityModeToWayland();
    updateStruts();

    Q_EMIT visibilityModeChanged();

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

PanelView::OpacityMode PanelView::opacityMode() const
{
    if (!m_theme.adaptiveTransparencyEnabled()) {
        return PanelView::Translucent;
    }
    return m_opacityMode;
}

bool PanelView::adaptiveOpacityEnabled()
{
    return m_theme.adaptiveTransparencyEnabled();
}

void PanelView::setOpacityMode(PanelView::OpacityMode mode)
{
    if (m_opacityMode != mode) {
        m_opacityMode = mode;
        if (config().isValid() && config().parent().isValid()) {
            config().parent().writeEntry("panelOpacity", (int)mode);
            m_corona->requestApplicationConfigSync();
        }
        Q_EMIT opacityModeChanged();
    }
}

void PanelView::updateAdaptiveOpacityEnabled()
{
    Q_EMIT opacityModeChanged();
    Q_EMIT adaptiveOpacityEnabledChanged();
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

    KWindowEffects::slideWindow(this, slideLocation, -1);
}

QRect PanelView::geometryByDistance(int distance) const
{
    if (!containment() || !m_screenToFollow) {
        return QRect();
    }

    QScreen *s = m_screenToFollow;
    const QRect screenGeometry = s->geometry();
    QRect r(QPoint(0, 0), formFactor() == Plasma::Types::Vertical ? QSize(totalThickness(), height()) : QSize(width(), totalThickness()));

    if (formFactor() == Plasma::Types::Horizontal) {
        switch (m_alignment) {
        case Qt::AlignCenter:
            r.moveCenter(screenGeometry.center() + QPoint(m_offset, 0));
            break;
        case Qt::AlignRight:
            r.moveRight(screenGeometry.right() - m_offset);
            break;
        case Qt::AlignLeft:
        default:
            r.moveLeft(screenGeometry.left() + m_offset);
        }
    } else {
        switch (m_alignment) {
        case Qt::AlignCenter:
            r.moveCenter(screenGeometry.center() + QPoint(0, m_offset));
            break;
        case Qt::AlignRight:
            r.moveBottom(screenGeometry.bottom() - m_offset);
            break;
        case Qt::AlignLeft:
        default:
            r.moveTop(screenGeometry.top() + m_offset);
        }
    }

    switch (containment()->location()) {
    case Plasma::Types::TopEdge:
        r.moveTop(screenGeometry.top() + distance);
        break;

    case Plasma::Types::LeftEdge:
        r.moveLeft(screenGeometry.left() + distance);
        break;

    case Plasma::Types::RightEdge:
        r.moveRight(screenGeometry.right() - distance);
        break;

    case Plasma::Types::BottomEdge:
    default:
        r.moveBottom(screenGeometry.bottom() - distance);
    }
    return r.intersected(screenGeometry);
}

void PanelView::resizePanel()
{
    if (!m_initCompleted) {
        return;
    }

    // On Wayland when a screen is disconnected and the panel is migrating to a newscreen
    // it can happen a moment where the qscreen gets destroyed before it gets reassigned
    // to the new screen
    if (!m_screenToFollow) {
        return;
    }

    QSize targetSize;
    QSize targetMinSize;
    QSize targetMaxSize;

    if (formFactor() == Plasma::Types::Vertical) {
        const int minSize = qMax(MINSIZE, m_minLength);
        int maxSize = qMin(m_maxLength, m_screenToFollow->size().height() - m_offset);
        maxSize = qMax(minSize, maxSize);
        targetMinSize = QSize(totalThickness(), minSize);
        targetMaxSize = QSize(totalThickness(), maxSize);
        targetSize = QSize(totalThickness(), std::clamp(m_contentLength + m_topFloatingPadding + m_bottomFloatingPadding, minSize, maxSize));
    } else {
        const int minSize = qMax(MINSIZE, m_minLength);
        int maxSize = qMin(m_maxLength, m_screenToFollow->size().width() - m_offset);
        maxSize = qMax(minSize, maxSize);
        targetMinSize = QSize(minSize, totalThickness());
        targetMaxSize = QSize(maxSize, totalThickness());
        targetSize = QSize(std::clamp(m_contentLength + m_leftFloatingPadding + m_rightFloatingPadding, minSize, maxSize), totalThickness());
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

    // position will be updated implicitly from resizeEvent
}

void PanelView::restore()
{
    KConfigGroup panelConfig = config();
    if (!panelConfig.isValid()) {
        return;
    }

    // All the defaults are based on whatever are the current values
    // so won't be weirdly reset after screen resolution change

    // alignment is not resolution dependent
    // but if fails read it from the resolution dependent one as
    // the place for this config key is changed in Plasma 5.9
    // Do NOT use readConfigValueWithFallBack
    setAlignment((Qt::Alignment)panelConfig.parent().readEntry<int>("alignment", panelConfig.readEntry<int>("alignment", m_alignment)));

    // All the other values are read from screen independent values,
    // but fallback on the screen independent section, as is the only place
    // is safe to directly write during plasma startup, as there can be
    // resolution changes
    m_offset = readConfigValueWithFallBack("offset", m_offset);
    if (m_alignment != Qt::AlignCenter) {
        m_offset = qMax(0, m_offset);
    }

    setFloating((bool)config().parent().readEntry<int>("floating", configDefaults().parent().readEntry<int>("floating", false)));
    setThickness(configDefaults().readEntry("thickness", m_thickness));

    const QSize screenSize = m_screenToFollow->size();
    setMinimumSize(QSize(-1, -1));
    // FIXME: an invalid size doesn't work with QWindows
    setMaximumSize(screenSize);

    m_initCompleted = true;
    positionPanel();

    const int side = containment()->formFactor() == Plasma::Types::Vertical ? screenSize.height() : screenSize.width();
    const int maxSize = side - m_offset;
    m_maxLength = qBound<int>(MINSIZE, readConfigValueWithFallBack("maxLength", side), maxSize);
    m_minLength = qBound<int>(MINSIZE, readConfigValueWithFallBack("minLength", side), maxSize);

    // panelVisibility is not resolution dependent
    // but if fails read it from the resolution dependent one as
    // the place for this config key is changed in Plasma 5.9
    // Do NOT use readConfigValueWithFallBack
    setVisibilityMode((VisibilityMode)panelConfig.parent().readEntry<int>("panelVisibility", panelConfig.readEntry<int>("panelVisibility", (int)NormalPanel)));
    setOpacityMode((OpacityMode)config().parent().readEntry<int>("panelOpacity",
                                                                 configDefaults().parent().readEntry<int>("panelOpacity", PanelView::OpacityMode::Adaptive)));
    resizePanel();

    Q_EMIT maximumLengthChanged();
    Q_EMIT minimumLengthChanged();
    Q_EMIT offsetChanged();
    Q_EMIT alignmentChanged();
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

    m_panelConfigView->init();
    m_panelConfigView->show();
    m_panelConfigView->requestActivate();

    if (isPanelConfig) {
        KWindowSystem::setState(m_panelConfigView->winId(), NET::SkipTaskbar | NET::SkipPager);
    }
}

void PanelView::restoreAutoHide()
{
    bool autoHide = true;
    disconnect(m_transientWindowVisibleWatcher);

    if (!edgeActivated()) {
        autoHide = false;
    } else if (m_containsMouse) {
        autoHide = false;
    } else if (containment() && containment()->isUserConfiguring()) {
        autoHide = false;
    } else if (containment() && containment()->status() >= Plasma::Types::NeedsAttentionStatus && containment()->status() != Plasma::Types::HiddenStatus) {
        autoHide = false;
    } else {
        for (QWindow *window : qApp->topLevelWindows()) {
            if (window->transientParent() == this && window->isVisible()) {
                m_transientWindowVisibleWatcher = connect(window, &QWindow::visibleChanged, this, &PanelView::restoreAutoHide);
                autoHide = false;
                break; // if multiple are open, we will re-evaluate this expression after the first closes
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

        UniqueCPointer<xcb_intern_atom_reply_t> atom(xcb_intern_atom_reply(c, atomCookie, nullptr));

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
        KWindowEffects::slideWindow(this, slideLocation, -1);
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
    // don't setGeometry() to make really sure we aren't doing a resize loop
    if (m_screenToFollow) {
        const QPoint pos = geometryByDistance(m_distance).topLeft();
        setPosition(pos);
        if (m_shellSurface) {
            m_shellSurface->setPosition(pos);
        }

        m_strutsTimer.start(STRUTSTIMERDELAY);
        Q_EMIT m_corona->availableScreenRegionChanged();
    }

    PlasmaQuick::ContainmentView::resizeEvent(ev);
}

void PanelView::moveEvent(QMoveEvent *ev)
{
    updateEnabledBorders();
    m_strutsTimer.start(STRUTSTIMERDELAY);
    PlasmaQuick::ContainmentView::moveEvent(ev);
    if (m_screenToFollow && !m_screenToFollow->geometry().contains(geometry())) {
        positionPanel();
    }
}

void PanelView::keyPressEvent(QKeyEvent *event)
{
    // Press escape key to cancel focus on the panel
    if (event->key() == Qt::Key_Escape) {
        if (containment()->status() == Plasma::Types::AcceptingInputStatus) {
            containment()->setStatus(Plasma::Types::PassiveStatus);
        }
        // No return for Wayland
    }

    PlasmaQuick::ContainmentView::keyPressEvent(event);
    if (event->isAccepted()) {
        return;
    }

    // Catch arrows keyPress to have same behavior as tab/backtab
    if ((event->key() == Qt::Key_Right && qApp->layoutDirection() == Qt::LeftToRight)
        || (event->key() == Qt::Key_Left && qApp->layoutDirection() != Qt::LeftToRight) || event->key() == Qt::Key_Down) {
        event->accept();
        QKeyEvent tabEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
        qApp->sendEvent((QObject *)this, (QEvent *)&tabEvent);
        return;
    } else if ((event->key() == Qt::Key_Right && qApp->layoutDirection() != Qt::LeftToRight)
               || (event->key() == Qt::Key_Left && qApp->layoutDirection() == Qt::LeftToRight) || event->key() == Qt::Key_Up) {
        event->accept();
        QKeyEvent backtabEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier);
        qApp->sendEvent((QObject *)this, (QEvent *)&backtabEvent);
        return;
    }
}

void PanelView::integrateScreen()
{
    updateMask();
    KX11Extras::setOnAllDesktops(winId(), true);
    KWindowSystem::setType(winId(), NET::Dock);
#if HAVE_X11
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QXcbWindowFunctions::setWmWindowType(this, QXcbWindowFunctions::Dock);
#else
    // QXcbWindow isn't installed and thus inaccessible to us, but it does read this magic property...
    setProperty("_q_xcb_wm_window_type", QNativeInterface::Private::QXcbWindow::Dock);
#endif
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

    if (!m_screenToFollow.isNull()) {
        // disconnect from old screen
        disconnect(m_screenToFollow, &QScreen::virtualGeometryChanged, this, &PanelView::updateStruts);
        disconnect(m_screenToFollow, &QScreen::geometryChanged, this, &PanelView::restore);
    }

    connect(screen, &QScreen::virtualGeometryChanged, this, &PanelView::updateStruts, Qt::UniqueConnection);
    connect(screen, &QScreen::geometryChanged, this, &PanelView::restore, Qt::UniqueConnection);

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
    Q_EMIT screenToFollowChanged(m_screenToFollow);
    m_lastScreen = m_screenToFollow;

    if (!m_screenToFollow) {
        return;
    }

    integrateScreen();
    showTemporarily();
    restore();
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

        // first, don't mess with position if the cursor is actually outside the view:
        // somebody is doing a click and drag that must not break when the cursor i outside
        if (geometry().contains(QCursor::pos(screenToFollow()))) {
            if (!containmentContainsPosition(me->windowPos()) && !m_fakeEventPending) {
                QMouseEvent me2(me->type(),
                                positionAdjustedForContainment(me->windowPos()),
                                positionAdjustedForContainment(me->windowPos()),
                                positionAdjustedForContainment(me->windowPos()) + position(),
                                me->button(),
                                me->buttons(),
                                me->modifiers());

                m_fakeEventPending = true;
                QCoreApplication::sendEvent(this, &me2);
                m_fakeEventPending = false;
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

        if (!containmentContainsPosition(we->position()) && !m_fakeEventPending) {
            QWheelEvent we2(positionAdjustedForContainment(we->position()),
                            positionAdjustedForContainment(we->position()) + position(),
                            we->pixelDelta(),
                            we->angleDelta(),
                            we->buttons(),
                            we->modifiers(),
                            we->phase(),
                            we->inverted());

            m_fakeEventPending = true;
            QCoreApplication::sendEvent(this, &we2);
            m_fakeEventPending = false;
            return true;
        }
        break;
    }

    case QEvent::DragEnter: {
        QDragEnterEvent *de = static_cast<QDragEnterEvent *>(e);
        if (!containmentContainsPosition(de->pos()) && !m_fakeEventPending) {
            QDragEnterEvent de2(positionAdjustedForContainment(de->pos()).toPoint(),
                                de->possibleActions(),
                                de->mimeData(),
                                de->mouseButtons(),
                                de->keyboardModifiers());

            m_fakeEventPending = true;
            QCoreApplication::sendEvent(this, &de2);
            m_fakeEventPending = false;
            return true;
        }
        break;
    }
    // DragLeave just works
    case QEvent::DragLeave:
        break;
    case QEvent::DragMove: {
        QDragMoveEvent *de = static_cast<QDragMoveEvent *>(e);
        if (!containmentContainsPosition(de->pos()) && !m_fakeEventPending) {
            QDragMoveEvent de2(positionAdjustedForContainment(de->pos()).toPoint(),
                               de->possibleActions(),
                               de->mimeData(),
                               de->mouseButtons(),
                               de->keyboardModifiers());

            m_fakeEventPending = true;
            QCoreApplication::sendEvent(this, &de2);
            m_fakeEventPending = false;
            return true;
        }
        break;
    }
    case QEvent::Drop: {
        QDropEvent *de = static_cast<QDropEvent *>(e);
        if (!containmentContainsPosition(de->pos()) && !m_fakeEventPending) {
            QDropEvent de2(positionAdjustedForContainment(de->pos()).toPoint(),
                           de->possibleActions(),
                           de->mimeData(),
                           de->mouseButtons(),
                           de->keyboardModifiers());

            m_fakeEventPending = true;
            QCoreApplication::sendEvent(this, &de2);
            m_fakeEventPending = false;
            return true;
        }
        break;
    }

    case QEvent::Hide: {
        if (m_panelConfigView && m_panelConfigView->isVisible()) {
            m_panelConfigView->hide();
        }
        m_containsMouse = false;
        break;
    }
    case QEvent::PlatformSurface:
        switch (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceCreated:
            setupWaylandIntegration();
            PanelShadows::self()->addWindow(this, enabledBorders());
            updateEnabledBorders();
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

    return QRectF(containmentItem->mapToScene(QPoint(m_leftPadding, m_topPadding)),
                  QSizeF(containmentItem->width() - m_leftPadding - m_rightPadding, containmentItem->height() - m_topPadding - m_bottomPadding))
        .contains(point);
}

QPointF PanelView::positionAdjustedForContainment(const QPointF &point) const
{
    QQuickItem *containmentItem = containment()->property("_plasma_graphicObject").value<QQuickItem *>();

    if (!containmentItem) {
        return point;
    }

    QRectF containmentRect(containmentItem->mapToScene(QPoint(0, 0)), QSizeF(containmentItem->width(), containmentItem->height()));

    // We are removing 1 to the e.g. containmentRect.right() - m_rightPadding because the last pixel would otherwise
    // the first one in the margin, and thus the mouse event would be discarded. Instead, the first pixel given by
    // containmentRect.left() + m_leftPadding the first one *not* in the margin, so it work.
    return QPointF(qBound(containmentRect.left() + m_leftPadding, point.x(), containmentRect.right() - m_rightPadding - 1),
                   qBound(containmentRect.top() + m_topPadding, point.y(), containmentRect.bottom() - m_bottomPadding - 1));
}

void PanelView::updateMask()
{
    if (m_backgroundHints == Plasma::Types::NoBackground) {
        KWindowEffects::enableBlurBehind(this, false);
        KWindowEffects::enableBackgroundContrast(this, false);
        setMask(QRegion());
    } else {
        QRegion mask;

        QQuickItem *rootObject = this->rootObject();
        if (rootObject) {
            const QVariant maskProperty = rootObject->property("panelMask");
            if (static_cast<QMetaType::Type>(maskProperty.type()) == QMetaType::QRegion) {
                mask = maskProperty.value<QRegion>();
                mask.translate(rootObject->property("maskOffsetX").toInt(), rootObject->property("maskOffsetY").toInt());
            }
        }
        KWindowEffects::enableBlurBehind(this, m_theme.blurBehindEnabled(), mask);
        KWindowEffects::enableBackgroundContrast(this,
                                                 m_theme.backgroundContrastEnabled(),
                                                 m_theme.backgroundContrast(),
                                                 m_theme.backgroundIntensity(),
                                                 m_theme.backgroundSaturation(),
                                                 mask);

        if (KX11Extras::compositingActive()) {
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

    // Extended struts against a screen edge near to another screen are really harmful, so windows maximized under the panel is a lesser pain
    // TODO: force "windows can cover" in those cases?
    const auto screenIds = m_corona->screenIds();
    for (int id : screenIds) {
        if (id == containment()->screen()) {
            continue;
        }

        const QRect otherScreen = corona()->screenGeometry(id);
        if (!otherScreen.isValid()) {
            continue;
        }

        switch (location()) {
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
            KX11Extras::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            return;
        }
        // extended struts are to the combined screen geoms, not the single screen
        int leftOffset = thisScreen.x();
        int rightOffset = wholeScreen.right() - thisScreen.right();
        int bottomOffset = wholeScreen.bottom() - thisScreen.bottom();
        //         qDebug() << "screen l/r/b/t offsets are:" << leftOffset << rightOffset << bottomOffset << topOffset << location();
        int topOffset = thisScreen.top();

        switch (location()) {
        case Plasma::Types::TopEdge:
            strut.top_width = totalThickness() + topOffset;
            strut.top_start = x();
            strut.top_end = x() + width() - 1;
            //                 qDebug() << "setting top edge to" << strut.top_width << strut.top_start << strut.top_end;
            break;

        case Plasma::Types::BottomEdge:
            strut.bottom_width = totalThickness() + bottomOffset;
            strut.bottom_start = x();
            strut.bottom_end = x() + width() - 1;
            //                 qDebug() << "setting bottom edge to" << strut.bottom_width << strut.bottom_start << strut.bottom_end;
            break;

        case Plasma::Types::RightEdge:
            strut.right_width = totalThickness() + rightOffset;
            strut.right_start = y();
            strut.right_end = y() + height() - 1;
            //                 qDebug() << "setting right edge to" << strut.right_width << strut.right_start << strut.right_end;
            break;

        case Plasma::Types::LeftEdge:
            strut.left_width = totalThickness() + leftOffset;
            strut.left_start = y();
            strut.left_end = y() + height() - 1;
            //                 qDebug() << "setting left edge to" << strut.left_width << strut.left_start << strut.left_end;
            break;

        default:
            // qDebug() << "where are we?";
            break;
        }
    }

    KX11Extras::setExtendedStrut(winId(),
                                 strut.left_width,
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

void PanelView::refreshContainment()
{
    restore();
    connect(containment(), &Plasma::Containment::userConfiguringChanged, this, [this](bool configuring) {
        if (configuring) {
            showTemporarily();
        } else {
            m_unhideTimer.start();
            updateStruts();
        }
    });

    connect(containment(), &Plasma::Applet::statusChanged, this, &PanelView::refreshStatus);
    connect(containment(), &Plasma::Applet::appletDeleted, this, [this] {
        // containment()->destroyed() is true only when the user deleted it
        // so the config is to be thrown away, not during shutdown
        if (containment()->destroyed()) {
            KConfigGroup views(m_corona->applicationConfig(), "PlasmaViews");
            for (auto grp : views.groupList()) {
                if (grp.contains(QRegularExpression(QStringLiteral("Panel %1$").arg(QString::number(containment()->id()))))) {
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
        disconnect(this, &QuickViewSharedEngine::statusChanged, this, &PanelView::handleQmlStatusChange);

        updatePadding();
        updateFloating();
        int paddingSignal = rootObject->metaObject()->indexOfSignal("bottomPaddingChanged()");
        if (paddingSignal >= 0) {
            connect(rootObject, SIGNAL(bottomPaddingChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(topPaddingChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(rightPaddingChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(leftPaddingChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(minPanelHeightChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(minPanelWidthChanged()), this, SLOT(updatePadding()));
        }
        const int floatingSignal = rootObject->metaObject()->indexOfSignal("bottomFloatingPaddingChanged()");
        if (floatingSignal >= 0) {
            connect(rootObject, SIGNAL(bottomFloatingPaddingChanged()), this, SLOT(updateFloating()));
            connect(rootObject, SIGNAL(topFloatingPaddingChanged()), this, SLOT(updateFloating()));
            connect(rootObject, SIGNAL(rightFloatingPaddingChanged()), this, SLOT(updateFloating()));
            connect(rootObject, SIGNAL(leftFloatingPaddingChanged()), this, SLOT(updateFloating()));
            connect(rootObject, SIGNAL(hasShadowsChanged()), this, SLOT(updateShadows()));
            connect(rootObject, SIGNAL(maskOffsetXChanged()), this, SLOT(updateMask()));
            connect(rootObject, SIGNAL(maskOffsetYChanged()), this, SLOT(updateMask()));
        }

        const QVariant maskProperty = rootObject->property("panelMask");
        if (static_cast<QMetaType::Type>(maskProperty.type()) == QMetaType::QRegion) {
            connect(rootObject, SIGNAL(panelMaskChanged()), this, SLOT(updateMask()));
            updateMask();
        }
    }
}

void PanelView::refreshStatus(Plasma::Types::ItemStatus status)
{
    if (status == Plasma::Types::NeedsAttentionStatus) {
        showTemporarily();
        setFlags(flags() | Qt::WindowDoesNotAcceptFocus);
        if (m_shellSurface) {
            m_shellSurface->setPanelTakesFocus(false);
        }
    } else if (status == Plasma::Types::AcceptingInputStatus) {
        setFlags(flags() & ~Qt::WindowDoesNotAcceptFocus);
#ifdef HAVE_X11
        KX11Extras::forceActiveWindow(winId());
#endif
        if (m_shellSurface) {
            m_shellSurface->setPanelTakesFocus(true);
        }
    } else {
        if (status == Plasma::Types::PassiveStatus) {
            m_corona->restorePreviousWindow();
        }

        restoreAutoHide();
        setFlags(flags() | Qt::WindowDoesNotAcceptFocus);
        if (m_shellSurface) {
            m_shellSurface->setPanelTakesFocus(false);
        }
    }
}

void PanelView::showTemporarily()
{
    setAutoHideEnabled(false);

    QTimer *t = new QTimer(this);
    t->setSingleShot(true);
    t->setInterval(3s);
    connect(t, &QTimer::timeout, this, &PanelView::restoreAutoHide);
    connect(t, &QTimer::timeout, t, &QObject::deleteLater);
    t->start();
}

void PanelView::screenDestroyed(QObject *)
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
    if (ShellCorona *c = qobject_cast<ShellCorona *>(corona())) {
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

        if (m_screenToFollow) {
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
    }

    if (m_enabledBorders != borders) {
        PanelShadows::self()->setEnabledBorders(this, borders);
        m_enabledBorders = borders;
        Q_EMIT enabledBordersChanged();
    }
}

void PanelView::updatePadding()
{
    if (!rootObject())
        return;
    m_leftPadding = rootObject()->property("leftPadding").toInt();
    m_rightPadding = rootObject()->property("rightPadding").toInt();
    m_topPadding = rootObject()->property("topPadding").toInt();
    m_bottomPadding = rootObject()->property("bottomPadding").toInt();
    m_minDrawingHeight = rootObject()->property("minPanelHeight").toInt();
    m_minDrawingWidth = rootObject()->property("minPanelWidth").toInt();
    Q_EMIT minThicknessChanged();
    setThickness(m_thickness);
}

void PanelView::updateShadows()
{
    if (!rootObject()) {
        return;
    }
    bool hasShadows = rootObject()->property("hasShadows").toBool();
    if (hasShadows) {
        PanelShadows::self()->addWindow(this, enabledBorders());
    } else {
        PanelShadows::self()->removeWindow(this);
    }
}

void PanelView::updateFloating()
{
    if (!rootObject()) {
        return;
    }
    m_leftFloatingPadding = rootObject()->property("leftFloatingPadding").toInt();
    m_rightFloatingPadding = rootObject()->property("rightFloatingPadding").toInt();
    m_topFloatingPadding = rootObject()->property("topFloatingPadding").toInt();
    m_bottomFloatingPadding = rootObject()->property("bottomFloatingPadding").toInt();

    positionPanel();
    resizePanel();
    updateMask();
}

#include "moc_panelview.cpp"
