/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-plasma.h>

#include <chrono>

#include "autohidescreenedge.h"
#include "debug.h"
#include "desktopview.h"
#include "panelconfigview.h"
#include "panelshadows_p.h"
#include "panelview.h"
#include "shellcorona.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQmlContext>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QScreen>

#include <KLocalizedString>
#include <KX11Extras>
#include <kwindowsystem.h>

#include <Plasma/Containment>
#include <PlasmaQuick/AppletQuickItem>

#include <LayerShellQt/Window>

#if HAVE_X11
#include <NETWM>
#include <qpa/qplatformwindow_p.h>
#endif

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

static const int MINSIZE = 10;

PanelView::PanelView(ShellCorona *corona, QScreen *targetScreen, QWindow *parent)
    : PlasmaQuick::ContainmentView(corona, parent)
    , m_offset(0)
    , m_maxLength(0)
    , m_minLength(0)
    , m_contentLength(0)
    , m_thickness(30)
    , m_minDrawingWidth(0)
    , m_minDrawingHeight(0)
    , m_initCompleted(false)
    , m_floating(false)
    , m_floatingApplets(false)
    , m_alignment(Qt::AlignCenter)
    , m_corona(corona)
    , m_visibilityMode(NormalPanel)
    , m_opacityMode(Adaptive)
    , m_lengthMode(FillAvailable)
    , m_backgroundHints(Plasma::Types::StandardBackground)
{
    if (KWindowSystem::isPlatformWayland()) {
        m_layerWindow = LayerShellQt::Window::get(this);
        m_layerWindow->setLayer(LayerShellQt::Window::LayerTop);
        m_layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        m_layerWindow->setScope(QStringLiteral("dock"));
        m_layerWindow->setCloseOnDismissed(false);
    }
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

    connect(m_corona, &Plasma::Corona::editModeChanged, this, &PanelView::restoreAutoHide);

    m_lastScreen = targetScreen;
    connect(this, &PanelView::containmentChanged, this, &PanelView::refreshContainment);
    connect(this, &PanelView::locationChanged, this, [this](Plasma::Types::Location location) {
        switch (location) {
        case Plasma::Types::LeftEdge:
        case Plasma::Types::RightEdge:
            containment()->setFormFactor(Plasma::Types::Vertical);
            break;
        case Plasma::Types::TopEdge:
        case Plasma::Types::BottomEdge:
            containment()->setFormFactor(Plasma::Types::Horizontal);
        }
        restore();
    });

    if (!m_corona->kPackage().isValid()) {
        qCWarning(PLASMASHELL) << "Invalid home screen package";
    }

    m_strutsTimer.setSingleShot(true);
    connect(&m_strutsTimer, &QTimer::timeout, this, &PanelView::updateExclusiveZone);

    connect(m_corona, &Plasma::Corona::editModeChanged, this, &PanelView::updateEditModeLabel);

    // Register enums
    qmlRegisterUncreatableMetaObject(PanelView::staticMetaObject, "org.kde.plasma.shell.panel", 0, 1, "Global", QStringLiteral("Error: only enums"));

    qmlRegisterAnonymousType<QScreen>("", 1);
    rootContext()->setContextProperty(QStringLiteral("panel"), this);
    setSource(m_corona->kPackage().fileUrl("views", QStringLiteral("Panel.qml")));
    updatePadding();
    updateFloating();
    updateTouchingWindow();
}

PanelView::~PanelView()
{
    if (containment()) {
        m_corona->requestApplicationConfigSync();
    }
}

static bool vendorIsNVidia()
{
    QOffscreenSurface surface;
    surface.create();
    QOpenGLContext context;
    context.create();
    if (context.makeCurrent(&surface)) {
        QOpenGLFunctions funcs(&context);
        const QString vendor = QString::fromLocal8Bit(reinterpret_cast<const char *>(funcs.glGetString(GL_VENDOR)));
        return vendor.contains(u"NVIDIA", Qt::CaseInsensitive);
    }
    return false;
}

bool PanelView::isUnsupportedEnvironment() const
{
    // See BUG 475468 on why is this needed.

    // It is a static variable: compute it once and cache the results because
    // we don't expect such configuration to change at runtime.
    static const bool unsupported = []() {
#if HAVE_X11
        const auto isX11 = KWindowSystem::isPlatformX11();
#else
        const auto isX11 = false;
#endif
        return isX11 && vendorIsNVidia();
    }();
    return unsupported;
}

bool PanelView::defaultFloating() const
{
    return isUnsupportedEnvironment() ? false : true;
}

PanelView::OpacityMode PanelView::defaultOpacityMode() const
{
    return isUnsupportedEnvironment() ? PanelView::OpacityMode::Translucent : PanelView::OpacityMode::Adaptive;
}

KConfigGroup PanelView::panelConfig(ShellCorona *corona, Plasma::Containment *containment, QScreen *screen)
{
    if (!containment || !screen) {
        return KConfigGroup();
    }
    KConfigGroup views(corona->applicationConfig(), u"PlasmaViews"_s);
    views = KConfigGroup(&views, QStringLiteral("Panel %1").arg(containment->id()));

    if (containment->formFactor() == Plasma::Types::Vertical) {
        return KConfigGroup(&views, QString(u"Vertical" + QString::number(screen->size().height())));
        // treat everything else as horizontal
    } else {
        return KConfigGroup(&views, QString(u"Horizontal" + QString::number(screen->size().width())));
    }
}

KConfigGroup PanelView::panelConfigDefaults(ShellCorona *corona, Plasma::Containment *containment, QScreen *screen)
{
    if (!containment || !screen) {
        return KConfigGroup();
    }

    KConfigGroup views(corona->applicationConfig(), u"PlasmaViews"_s);
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
    Q_EMIT m_corona->availableScreenRegionChanged(containment()->screen());
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
    positionAndResizePanel();
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

    positionAndResizePanel();
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

    positionAndResizePanel();
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

    positionAndResizePanel();
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
    Q_EMIT unsupportedConfigurationChanged();

    updateFloating();
    updateEnabledBorders();
    updateMask();
}

bool PanelView::floatingApplets() const
{
    return m_floatingApplets;
}

void PanelView::setFloatingApplets(bool floatingApplets)
{
    if (m_floatingApplets == floatingApplets) {
        return;
    }

    m_floatingApplets = floatingApplets;
    if (config().isValid() && config().parent().isValid()) {
        config().parent().writeEntry("floatingApplets", (int)floatingApplets);
        m_corona->requestApplicationConfigSync();
    }
    Q_EMIT floatingAppletsChanged();
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

bool PanelView::isUnsupportedConfiguration() const
{
    return isUnsupportedEnvironment() && (floating() != defaultFloating() || opacityMode() == Adaptive);
}

QString PanelView::unsupportedConfigurationDescription() const
{
    if (isUnsupportedConfiguration()) {
        return i18n(
            "With an NVIDIA GPU on X11, the Floating style and Adaptive opacity mode are known to cause poor window drag and resize performance. Consider "
            "using other settings.");
    }
    return QString();
}

void PanelView::fixUnsupportedConfiguration()
{
    if (isUnsupportedConfiguration()) {
        setFloating(defaultFloating());
        setOpacityMode(defaultOpacityMode());
    }
}

void PanelView::clonePanelTo(Plasma::Types::Location location, QScreen *screen)
{
    m_corona->clonePanelTo(this, location, screen);
}

QRect PanelView::relativeConfigRect() const
{
    if (!m_screenToFollow) {
        return QRect();
    }
    PanelConfigView *panelConfigView = qobject_cast<PanelConfigView *>(m_panelConfigView);
    if (!panelConfigView || !panelConfigView->isVisible()) {
        return QRect();
    }
    const QRect screenGeo = m_screenToFollow->geometry();
    QRect rect = m_panelConfigView->geometry();
    rect.moveTopLeft(QPoint(rect.left() - screenGeo.left(), rect.top() - screenGeo.top()));
    return rect;
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

KSvg::FrameSvg::EnabledBorders PanelView::enabledBorders() const
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

    updateExclusiveZone();

    Q_EMIT visibilityModeChanged();

    restoreAutoHide();
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

PanelView::LengthMode PanelView::lengthMode() const
{
    return m_lengthMode;
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
        Q_EMIT unsupportedConfigurationChanged();
    }
}

void PanelView::setLengthMode(PanelView::LengthMode mode)
{
    if (m_lengthMode != mode) {
        m_lengthMode = mode;
        if (config().isValid() && config().parent().isValid()) {
            config().parent().writeEntry("panelLengthMode", (int)mode);
            m_corona->requestApplicationConfigSync();
        }
        Q_EMIT lengthModeChanged();
        positionAndResizePanel();
        Q_EMIT m_corona->availableScreenRegionChanged(containment()->screen());
    }
}

void PanelView::updateAdaptiveOpacityEnabled()
{
    Q_EMIT opacityModeChanged();
    Q_EMIT adaptiveOpacityEnabledChanged();
}

KWindowEffects::SlideFromLocation PanelView::slideLocation() const
{
    switch (containment()->location()) {
    case Plasma::Types::TopEdge:
        containment()->setFormFactor(Plasma::Types::Horizontal);
        return KWindowEffects::TopEdge;

    case Plasma::Types::LeftEdge:
        containment()->setFormFactor(Plasma::Types::Vertical);
        return KWindowEffects::LeftEdge;

    case Plasma::Types::RightEdge:
        containment()->setFormFactor(Plasma::Types::Vertical);
        return KWindowEffects::RightEdge;

    case Plasma::Types::BottomEdge:
    default:
        containment()->setFormFactor(Plasma::Types::Horizontal);
        return KWindowEffects::BottomEdge;
    }
}

void PanelView::updateLayerWindow()
{
    if (!m_layerWindow) {
        return;
    }

    QMargins margins;
    LayerShellQt::Window::Anchors anchors;
    LayerShellQt::Window::Anchor edge;

    switch (containment()->location()) {
    case Plasma::Types::TopEdge:
        anchors.setFlag(LayerShellQt::Window::AnchorTop);
        edge = LayerShellQt::Window::AnchorTop;
        break;
    case Plasma::Types::LeftEdge:
        anchors.setFlag(LayerShellQt::Window::AnchorLeft);
        edge = LayerShellQt::Window::AnchorLeft;
        break;
    case Plasma::Types::RightEdge:
        anchors.setFlag(LayerShellQt::Window::AnchorRight);
        edge = LayerShellQt::Window::AnchorRight;
        break;
    case Plasma::Types::BottomEdge:
    default:
        anchors.setFlag(LayerShellQt::Window::AnchorBottom);
        edge = LayerShellQt::Window::AnchorBottom;
        break;
    }

    if (formFactor() == Plasma::Types::Horizontal) {
        switch (m_alignment) {
        case Qt::AlignLeft:
            anchors.setFlag(LayerShellQt::Window::AnchorLeft);
            if (m_lengthMode == PanelView::LengthMode::Custom) {
                margins.setLeft(margins.left() + m_offset);
            }
            break;
        case Qt::AlignCenter:
            break;
        case Qt::AlignRight:
            anchors.setFlag(LayerShellQt::Window::AnchorRight);
            if (m_lengthMode == PanelView::LengthMode::Custom) {
                margins.setRight(margins.right() + m_offset);
            }
            break;
        }
        if (m_lengthMode == PanelView::LengthMode::FillAvailable) {
            anchors.setFlag(LayerShellQt::Window::AnchorLeft);
            anchors.setFlag(LayerShellQt::Window::AnchorRight);
        }
    } else {
        switch (m_alignment) {
        case Qt::AlignLeft:
            anchors.setFlag(LayerShellQt::Window::AnchorTop);
            if (m_lengthMode == PanelView::LengthMode::Custom) {
                margins.setTop(margins.top() + m_offset);
            }
            break;
        case Qt::AlignCenter:
            break;
        case Qt::AlignRight:
            anchors.setFlag(LayerShellQt::Window::AnchorBottom);
            if (m_lengthMode == PanelView::LengthMode::Custom) {
                margins.setBottom(margins.bottom() + m_offset);
            }
            break;
        }
        if (m_lengthMode == PanelView::LengthMode::FillAvailable) {
            anchors.setFlag(LayerShellQt::Window::AnchorTop);
            anchors.setFlag(LayerShellQt::Window::AnchorBottom);
        }
    }

    m_layerWindow->setAnchors(anchors);
    m_layerWindow->setExclusiveEdge(edge);
    m_layerWindow->setMargins(margins);

    requestUpdate();
}

void PanelView::positionPanel()
{
    if (!containment()) {
        return;
    }

    if (!m_initCompleted) {
        return;
    }

    updateLayerWindow();

    // TODO: Make it X11-specific. It's still relevant on wayland because of popup positioning.
    const QPoint pos = geometryByDistance(0).topLeft();
    setPosition(pos);
    updateMask();
    Q_EMIT geometryChanged();
    Q_EMIT m_corona->availableScreenRegionChanged(containment()->screen());

    KWindowEffects::slideWindow(this, slideLocation(), -1);
}

void PanelView::positionAndResizePanel()
{
    if (!containment()) {
        return;
    }

    if (!m_initCompleted) {
        return;
    }

    const QSize sizeHint = preferredSize();
    if (sizeHint.isEmpty()) {
        return;
    }

    // TODO: Make it X11-specific. It's still relevant on wayland because of popup positioning.
    const QPoint pos = geometryByDistance(0).topLeft();

    const QRect geom = {pos, sizeHint};

    if (geom == geometry()) {
        return;
    }

    updateLayerWindow();

    // At least one QWindow setGeometry is needed to avoid a protocol error
    if (m_layerWindow && !size().isEmpty()) {
        m_layerWindow->setDesiredSize(geom.size());
        setPosition(geom.topLeft());
    } else {
        setGeometry(geom);
    }
    updateMask();
    Q_EMIT geometryChanged();
    Q_EMIT m_corona->availableScreenRegionChanged(containment()->screen());

    KWindowEffects::slideWindow(this, slideLocation(), -1);
}

QRect PanelView::dogdeGeometryByDistance(int distance) const
{
    if (!containment() || !m_screenToFollow) {
        return QRect();
    }
    const QRect dodgeGeometry = geometryByDistance(distance);
    return dodgeGeometry & m_screenToFollow->geometry();
}

QRect PanelView::geometryByDistance(int distance) const
{
    if (!containment() || !m_screenToFollow) {
        return QRect();
    }

    const QRect screenGeometry = m_screenToFollow->geometry();
    QRect r(QPoint(0, 0),
            formFactor() == Plasma::Types::Vertical ? QSize(totalThickness(), preferredSize().height()) : QSize(preferredSize().width(), totalThickness()));

    int currentOffset = 0;
    if (m_lengthMode == PanelView::LengthMode::Custom) {
        currentOffset = m_offset;
    }

    if (formFactor() == Plasma::Types::Horizontal) {
        switch (m_alignment) {
        case Qt::AlignCenter:
            r.moveCenter(screenGeometry.center() + QPoint(currentOffset, 0));
            break;
        case Qt::AlignRight:
            r.moveRight(screenGeometry.right() - currentOffset);
            break;
        case Qt::AlignLeft:
        default:
            r.moveLeft(screenGeometry.left() + currentOffset);
        }
    } else {
        switch (m_alignment) {
        case Qt::AlignCenter:
            r.moveCenter(screenGeometry.center() + QPoint(0, currentOffset));
            break;
        case Qt::AlignRight:
            r.moveBottom(screenGeometry.bottom() - currentOffset);
            break;
        case Qt::AlignLeft:
        default:
            r.moveTop(screenGeometry.top() + currentOffset);
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
    return r;
}

QSize PanelView::preferredSize() const
{
    if (!m_initCompleted) {
        return QSize();
    }

    // On Wayland when a screen is disconnected and the panel is migrating to a newscreen
    // it can happen a moment where the qscreen gets destroyed before it gets reassigned
    // to the new screen
    if (!m_screenToFollow) {
        return QSize();
    }

    QSize targetSize;
    if (m_lengthMode == PanelView::LengthMode::FillAvailable) {
        if (formFactor() == Plasma::Types::Vertical) {
            targetSize = QSize(totalThickness(), m_screenToFollow->size().height());
        } else {
            targetSize = QSize(m_screenToFollow->size().width(), totalThickness());
        }
    } else if (m_lengthMode == PanelView::LengthMode::FitContent) {
        if (formFactor() == Plasma::Types::Vertical) {
            targetSize = QSize(
                totalThickness(),
                std::max(totalThickness(), std::min(m_screenToFollow->size().height(), m_contentLength + m_topFloatingPadding + m_bottomFloatingPadding)));
        } else {
            targetSize =
                QSize(std::max(totalThickness(), std::min(m_screenToFollow->size().width(), m_contentLength + m_leftFloatingPadding + m_rightFloatingPadding)),
                      totalThickness());
        }
    } else {
        // PanelView::LengthMode::Custom
        if (formFactor() == Plasma::Types::Vertical) {
            const int minSize = qMax(MINSIZE, m_minLength);
            int maxSize = qMin(m_maxLength, m_screenToFollow->size().height() - m_offset);
            maxSize = qMax(minSize, maxSize);
            targetSize = QSize(totalThickness(), std::clamp(m_contentLength + m_topFloatingPadding + m_bottomFloatingPadding, minSize, maxSize));
        } else {
            const int minSize = qMax(MINSIZE, m_minLength);
            int maxSize = qMin(m_maxLength, m_screenToFollow->size().width() - m_offset);
            maxSize = qMax(minSize, maxSize);
            targetSize = QSize(std::clamp(m_contentLength + m_leftFloatingPadding + m_rightFloatingPadding, minSize, maxSize), totalThickness());
        }
    }

    return targetSize;
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
    m_offset = readConfigValueWithFallBack(u"offset"_s, m_offset);
    if (m_alignment == Qt::AlignCenter) {
        m_offset = 0;
    } else {
        m_offset = qMax(0, m_offset);
    }

    setFloating((bool)config().parent().readEntry<int>("floating", defaultFloating()));
    setFloatingApplets((bool)config().parent().readEntry<int>("floatingApplets", false));
    setThickness(configDefaults().readEntry("thickness", m_thickness));

    const QSize screenSize = m_screenToFollow->size();

    m_initCompleted = true;

    const int side = containment()->formFactor() == Plasma::Types::Vertical ? screenSize.height() : screenSize.width();
    const int maxSize = side - m_offset;
    m_maxLength = qBound<int>(MINSIZE, readConfigValueWithFallBack(u"maxLength"_s, side), maxSize);
    m_minLength = qBound<int>(MINSIZE, readConfigValueWithFallBack(u"minLength"_s, side), maxSize);

    // panelVisibility is not resolution dependent
    // but if fails read it from the resolution dependent one as
    // the place for this config key is changed in Plasma 5.9
    // Do NOT use readConfigValueWithFallBack
    setVisibilityMode((VisibilityMode)panelConfig.parent().readEntry<int>("panelVisibility", panelConfig.readEntry<int>("panelVisibility", (int)NormalPanel)));
    setOpacityMode((OpacityMode)config().parent().readEntry<int>("panelOpacity", defaultOpacityMode()));
    setLengthMode((LengthMode)config().parent().readEntry<int>("panelLengthMode", PanelView::LengthMode::FillAvailable));
    positionAndResizePanel();

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

    if (isPanelConfig) {
        if (m_panelConfigView && m_panelConfigView->isVisible()) {
            m_panelConfigView->hide();

            // We only exit edit mode if there's no other containment
            // being configured.

            if (!m_corona->enteredEditModeViaDesktop()) {
                m_corona->setEditMode(false);
            }

        } else if (m_panelConfigView) {
            m_panelConfigView->show();
            m_panelConfigView->requestActivate();
        } else {
            PanelConfigView *configView = new PanelConfigView(cont, this);
            connect(configView, &PanelConfigView::visibleChanged, this, &PanelView::relativeConfigRectChanged);
            connect(configView, &PanelConfigView::visibleChanged, this, &PanelView::userConfiguringChanged);
            connect(configView, &PanelConfigView::geometryChanged, this, &PanelView::relativeConfigRectChanged);
            configView->setFlags(configView->flags() | Qt::Dialog | Qt::WindowStaysOnTopHint);

            configView->setAnimated(true);
            m_panelConfigView = configView;

            connect(this, &PanelView::formFactorChanged, this, [&]() {
                // The panel window will resize first, and the content
                // will follow. We first unset the visual parent so that
                // the config view does not reposition in the incorrect
                // position before the content has a chance to update.
                m_panelConfigView->setVisualParent(nullptr);
                // We will then re-parent the config view to the correct
                // item as soon as the window is done resizing.
                QTimer::singleShot(0, this, &PanelView::positionConfigView);
            });
            positionConfigView();

            configView->init();
            configView->show();
            configView->requestActivate();
            connect(m_panelConfigView, &PanelConfigView::visibleChanged, this, &PanelView::updateEditModeLabel);
            updateEditModeLabel();
        }
    } else {
        containment()->corona()->setEditMode(false);
        if (m_appletConfigView && m_appletConfigView->applet() == applet) {
            m_appletConfigView->show();
            m_appletConfigView->requestActivate();
            return;
        } else if (m_appletConfigView) {
            m_appletConfigView->deleteLater(); // BUG 476968
        }
        m_appletConfigView = new PlasmaQuick::ConfigView(applet);
        m_appletConfigView->init();
        m_appletConfigView->show();
        m_appletConfigView->requestActivate();
    }
}

void PanelView::positionConfigView()
{
    QQuickItem *contObject = PlasmaQuick::AppletQuickItem::itemForApplet(containment());
    QQuickItem *tb = contObject->property("toolBox").value<QQuickItem *>();
    if (tb && containment()->formFactor() != Plasma::Types::Vertical) {
        m_panelConfigView->setVisualParent(tb);
    } else {
        m_panelConfigView->setVisualParent(contObject);
    }
}

void PanelView::restoreAutoHide()
{
    bool autoHide = true;
    disconnect(m_transientWindowVisibleWatcher);

    if (!edgeActivated()) {
        autoHide = false;
    } else if (m_containsMouse || m_containsDrag) {
        autoHide = false;
    } else if (m_corona->isEditMode()) {
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
    if (m_visibilityMode == VisibilityMode::AutoHide || (m_visibilityMode == VisibilityMode::DodgeWindows && m_touchingWindow)) {
        if (!m_autoHideScreenEdge) {
            m_autoHideScreenEdge = AutoHideScreenEdge::create(this);
        }
        if (enabled) {
            m_autoHideScreenEdge->activate();
        } else {
            m_autoHideScreenEdge->deactivate();
        }
    } else {
        delete m_autoHideScreenEdge;
        m_autoHideScreenEdge = nullptr;
    }
}

void PanelView::resizeEvent(QResizeEvent *ev)
{
    updateEnabledBorders();

    // don't setGeometry() to make really sure we aren't doing a resize loop
    if (m_screenToFollow && containment()) {
        updateEnabledBorders();
        // TODO: Make it X11-specific. It's still relevant on wayland because of popup positioning.
        const QPoint pos = geometryByDistance(0).topLeft();
        setPosition(pos);
        Q_EMIT m_corona->availableScreenRegionChanged(containment()->screen());

        m_strutsTimer.start(STRUTSTIMERDELAY);
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
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::setOnAllDesktops(winId(), true);
        KX11Extras::setType(winId(), NET::Dock);
        if (auto xcbWindow = nativeInterface<QNativeInterface::Private::QXcbWindow>()) {
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Dock);
        }
    }
#endif
    setVisibilityMode(m_visibilityMode);

    if (containment()) {
        containment()->reactToScreenChange();
    }
}

void PanelView::updateEditModeLabel()
{
    bool editMode = containment()->corona()->isEditMode();
    QAction *action = containment()->internalAction(u"configure"_s);
    if (!action) {
        return;
    }
    if (m_panelConfigView && m_panelConfigView->isVisible() && editMode) {
        action->setText(i18nc("@action:inmenu", "Hide Panel Configuration"));
    } else {
        action->setText(i18nc("@action:inmenu", "Show Panel Configuration"));
    }
}

void PanelView::showEvent(QShowEvent *event)
{
    PlasmaQuick::ContainmentView::showEvent(event);

    integrateScreen();
    updateEditModeLabel();
}

void PanelView::setScreenToFollow(QScreen *screen)
{
    if (screen == m_screenToFollow) {
        return;
    }

    if (!screen) {
        return;
    }

    // layer surfaces can't be moved between outputs, so hide and show the window on a new output
    const bool remap = m_layerWindow && isVisible();
    if (remap) {
        setVisible(false);
    }

    if (!m_screenToFollow.isNull()) {
        // disconnect from old screen
        disconnect(m_screenToFollow, &QScreen::virtualGeometryChanged, this, &PanelView::updateExclusiveZone);
        disconnect(m_screenToFollow, &QScreen::geometryChanged, this, &PanelView::restore);
    }

    connect(screen, &QScreen::virtualGeometryChanged, this, &PanelView::updateExclusiveZone, Qt::UniqueConnection);
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

    if (remap) {
        setVisible(true);
    }
}

QScreen *PanelView::screenToFollow() const
{
    return m_screenToFollow;
}

bool PanelView::isUserConfiguring() const
{
    return m_panelConfigView && m_panelConfigView->isVisible();
}

void PanelView::adaptToScreen()
{
    Q_EMIT screenToFollowChanged(m_screenToFollow);
    m_lastScreen = m_screenToFollow;

    if (!m_screenToFollow) {
        return;
    }

    integrateScreen();
    restore();
}

bool PanelView::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Show:
        positionAndResizePanel();
        break;
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
        // BUG 482580: use QRectF for HiDPI
        if (QRectF(geometry()).contains(mapToGlobal(me->scenePosition()))) {
            if (!containmentContainsPosition(me->scenePosition()) && !m_fakeEventPending) {
                QMouseEvent me2(me->type(),
                                positionAdjustedForContainment(me->scenePosition()),
                                positionAdjustedForContainment(me->scenePosition()),
                                positionAdjustedForContainment(me->scenePosition()) + position(),
                                me->button(),
                                me->buttons(),
                                me->modifiers());
                me2.setTimestamp(me->timestamp());

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
            we2.setTimestamp(we->timestamp());

            m_fakeEventPending = true;
            QCoreApplication::sendEvent(this, &we2);
            m_fakeEventPending = false;
            return true;
        }
        break;
    }

    case QEvent::DragEnter: {
        m_containsDrag = true;
        if (edgeActivated()) {
            m_unhideTimer.stop();
        }
        QDragEnterEvent *de = static_cast<QDragEnterEvent *>(e);
        if (!containmentContainsPosition(de->position()) && !m_fakeEventPending) {
            QDragEnterEvent de2(positionAdjustedForContainment(de->position()).toPoint(),
                                de->possibleActions(),
                                de->mimeData(),
                                de->buttons(),
                                de->modifiers());

            m_fakeEventPending = true;
            QCoreApplication::sendEvent(this, &de2);
            m_fakeEventPending = false;
            return true;
        }
        break;
    }
    case QEvent::DragLeave:
        m_containsDrag = false;
        m_containsMouse = false;
        if (edgeActivated()) {
            m_unhideTimer.start();
        }
        break;
    case QEvent::DragMove: {
        QDragMoveEvent *de = static_cast<QDragMoveEvent *>(e);
        if (!containmentContainsPosition(de->position()) && !m_fakeEventPending) {
            QDragMoveEvent de2(positionAdjustedForContainment(de->position()).toPoint(), de->possibleActions(), de->mimeData(), de->buttons(), de->modifiers());

            m_fakeEventPending = true;
            QCoreApplication::sendEvent(this, &de2);
            m_fakeEventPending = false;
            return true;
        }
        break;
    }
    case QEvent::Drop: {
        m_containsDrag = false;
        QDropEvent *de = static_cast<QDropEvent *>(e);
        if (!containmentContainsPosition(de->position()) && !m_fakeEventPending) {
            QDropEvent de2(positionAdjustedForContainment(de->position()).toPoint(), de->possibleActions(), de->mimeData(), de->buttons(), de->modifiers());

            m_fakeEventPending = true;
            QCoreApplication::sendEvent(this, &de2);
            m_fakeEventPending = false;
            return true;
        }
        break;
    }

    case QEvent::Hide: {
        m_containsMouse = false;
        break;
    }
    default:
        break;
    }

    bool rc = ContainmentView::event(e);

    // PanelView does not take focus, but we want any users tracking focus
    // to act like they've lost focus when the panel is clicked and it was not used by another other mouse item
    if (e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(e);
        if (!me->allPointsGrabbed()) {
            QWindow *focusWindow = qGuiApp->focusWindow();
            if (focusWindow && focusWindow != this && focusWindow->flags().testFlag(Qt::FramelessWindowHint)) {
                // postEvent takes ownership of the event
                QFocusEvent *fe = new QFocusEvent(QEvent::FocusOut);
                qGuiApp->postEvent(focusWindow, fe);
            }
        }
    }

    return rc;
}

bool PanelView::containmentContainsPosition(const QPointF &point) const
{
    QQuickItem *containmentItem = PlasmaQuick::AppletQuickItem::itemForApplet(containment());

    if (!containmentItem) {
        return false;
    }

    QRectF boundingRect = QRectF(containmentItem->mapToScene(QPoint(m_leftPadding, m_topPadding)),
                                 QSizeF(containmentItem->width() - m_leftPadding - m_rightPadding, containmentItem->height() - m_topPadding - m_bottomPadding));

    // We are not using .contains here since that function will consider
    // boundingRect.x() + boundingRect.width() to be part of the rectangle,
    // but the right/bottom-most pixels do not react to user input. Thus, we
    // manually exclude them here.
    return (point.x() >= boundingRect.x() && point.x() < (boundingRect.x() + boundingRect.width()) && point.y() >= boundingRect.y()
            && point.y() < (boundingRect.y() + boundingRect.height()));
}

QPointF PanelView::positionAdjustedForContainment(const QPointF &point) const
{
    QQuickItem *containmentItem = PlasmaQuick::AppletQuickItem::itemForApplet(containment());

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
    if (!containment()) {
        return;
    }

    // Popups now align to the mask, without it they appear in the wrong position
    // always create it and show blur and contrast when needed
    QRegion mask;
    QQuickItem *rootObject = this->rootObject();
    QRect screenPanelRect = geometry();
    screenPanelRect.moveTo(mapFromGlobal(screenPanelRect.topLeft()));
    if (rootObject) {
        QVariant maskProperty = rootObject->property("panelMask");
        if (static_cast<QMetaType::Type>(maskProperty.typeId()) == QMetaType::QRegion) {
            mask = get<QRegion>(std::move(maskProperty));
            const QPoint floatingTranslucentItemOffset = rootObject->property("floatingTranslucentItemOffset").toPoint();
            mask.translate(floatingTranslucentItemOffset);
        }
    }

    if (mask.isEmpty()) {
        mask = QRegion(QRect(screenPanelRect));
    }

    if (m_backgroundHints == Plasma::Types::NoBackground) {
        KWindowEffects::enableBlurBehind(this, false);
        KWindowEffects::enableBackgroundContrast(this, false);
    } else {
        // We use mask for graphical effect which tightly  covers the panel
        // For the input region (QWindow::mask) screenPanelRect includes area around the floating
        // panel in order to respect Fitt's law
        KWindowEffects::enableBlurBehind(this, m_theme.blurBehindEnabled(), mask);
        KWindowEffects::enableBackgroundContrast(this,
                                                 m_theme.backgroundContrastEnabled(),
                                                 m_theme.backgroundContrast(),
                                                 m_theme.backgroundIntensity(),
                                                 m_theme.backgroundSaturation(),
                                                 mask);
    }

    if (!KWindowSystem::isPlatformX11() || KX11Extras::compositingActive()) {
        const QRect bounding = mask.boundingRect();
        // Always go to screen edge, to preserve fitts law
        switch (containment()->location()) {
        case Plasma::Types::LeftEdge:
            screenPanelRect.setRight(bounding.right());
            break;
        case Plasma::Types::TopEdge:
            screenPanelRect.setBottom(bounding.bottom());
            break;
        case Plasma::Types::RightEdge:
            screenPanelRect.setLeft(bounding.left());
            break;
        case Plasma::Types::BottomEdge:
            screenPanelRect.setTop(bounding.top());
            break;
        default:
            break;
        }
        setMask(screenPanelRect);
    } else {
        setMask(mask);
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
    NETRootInfo rootInfo(qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection(), NET::Supported | NET::SupportingWMCheck);
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

void PanelView::updateExclusiveZone()
{
    if (containment() && containment()->isUserConfiguring() && m_layerWindow && m_layerWindow->exclusionZone() == -1) {
        // We set the exclusive zone to make sure the ruler does not
        // overlap with the panel regardless of the visibility mode;
        // this won't be updated anymore as long as we are within
        // the panel configuration.
        switch (containment()->formFactor()) {
        case Plasma::Types::Horizontal:
            m_layerWindow->setExclusiveZone(thickness() + m_topFloatingPadding + m_bottomFloatingPadding);
            break;
        case Plasma::Types::Vertical:
            m_layerWindow->setExclusiveZone(thickness() + m_leftFloatingPadding + m_rightFloatingPadding);
            break;
        default:
            qWarning() << "Warning: unexpected panel formFactor:" << containment()->formFactor() << "Horizontal or Vertical expected";
        }
    }
    if (!containment() || containment()->isUserConfiguring() || !m_screenToFollow) {
        return;
    }

    if (KWindowSystem::isPlatformWayland()) {
        switch (m_visibilityMode) {
        case NormalPanel:
            if (m_corona->shouldPanelReserveSpace(this)) {
                m_layerWindow->setExclusiveZone(thickness());
            } else {
                m_layerWindow->setExclusiveZone(-1);
            }
            break;
        case AutoHide:
        case DodgeWindows:
        case WindowsGoBelow:
            m_layerWindow->setExclusiveZone(-1);
            break;
        }
        requestUpdate();
    } else {
#if HAVE_X11
        qreal top_width = 0, top_start = 0, top_end = 0;
        qreal bottom_width = 0, bottom_start = 0, bottom_end = 0;
        qreal right_width = 0, right_start = 0, right_end = 0;
        qreal left_width = 0, left_start = 0, left_end = 0;

        if (m_visibilityMode == NormalPanel) {
            if (!canSetStrut()) {
                KX11Extras::setExtendedStrut(winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                return;
            }

            // When setting struts, all arguments must belong to the logical coordinates.
            const double devicePixelRatio = m_screenToFollow->devicePixelRatio();
            const QRectF thisScreen{m_screenToFollow->geometry().topLeft().toPointF() / devicePixelRatio, m_screenToFollow->geometry().size()};
            // extended struts are to the combined screen geoms, not the single screen
            QRectF wholeScreen;
            for (const auto screens = qGuiApp->screens(); auto screen : screens) {
                const QRectF geometry = screen->geometry().toRectF();
                wholeScreen |= QRectF(geometry.topLeft() / devicePixelRatio, geometry.size());
            }

            const qreal offset = 1 / devicePixelRatio; // To make sure strut is only in a screen

            switch (location()) {
            case Plasma::Types::TopEdge: {
                const qreal topOffset = thisScreen.top();
                top_width = thickness() + topOffset;
                top_start = x() / devicePixelRatio;
                top_end = top_start + width() - offset;
                //                 qDebug() << "setting top edge to" << top_width << top_start << top_end;
                break;
            }

            case Plasma::Types::BottomEdge: {
                const qreal bottomOffset = wholeScreen.bottom() - thisScreen.bottom();
                bottom_width = thickness() + bottomOffset;
                bottom_start = x() / devicePixelRatio;
                bottom_end = bottom_start + width() - offset;
                //                 qDebug() << "setting bottom edge to" << bottom_width << bottom_start << bottom_end;
                break;
            }

            case Plasma::Types::RightEdge: {
                const qreal rightOffset = wholeScreen.right() - thisScreen.right();
                right_width = thickness() + rightOffset;
                right_start = y() / devicePixelRatio;
                right_end = right_start + height() - offset;
                //                 qDebug() << "setting right edge to" << right_width << right_start << right_end;
                break;
            }

            case Plasma::Types::LeftEdge: {
                const qreal leftOffset = thisScreen.x();
                left_width = thickness() + leftOffset;
                left_start = y() / devicePixelRatio;
                left_end = left_start + height() - offset;
                //                 qDebug() << "setting left edge to" << left_width << left_start << left_end;
                break;
            }

            default:
                // qDebug() << "where are we?";
                break;
            }
        }

        KX11Extras::setExtendedStrut(winId(),
                                     left_width,
                                     left_start,
                                     left_end,
                                     right_width,
                                     right_start,
                                     right_end,
                                     top_width,
                                     top_start,
                                     top_end,
                                     bottom_width,
                                     bottom_start,
                                     bottom_end);
#endif
    }
}

void PanelView::refreshContainment()
{
    restore();
    Plasma::Containment *const cont = containment();
    connect(cont, &Plasma::Containment::userConfiguringChanged, this, [this](bool configuring) {
        updateExclusiveZone();
        if (configuring) {
            showTemporarily();
        } else {
            m_unhideTimer.start();
        }
    });

    connect(cont, &Plasma::Applet::statusChanged, this, &PanelView::refreshStatus);
    connect(cont, &Plasma::Applet::appletDeleted, this, [this, cont] {
        // containment()->destroyed() is true only when the user deleted it
        // so the config is to be thrown away, not during shutdown
        if (cont->destroyed()) {
            KConfigGroup views(m_corona->applicationConfig(), u"PlasmaViews"_s);
            for (auto grp : views.groupList()) {
                if (grp.contains(QRegularExpression(QStringLiteral("Panel %1$").arg(QString::number(cont->id()))))) {
                    qDebug() << "Panel" << cont->id() << "removed by user";
                    views.deleteGroup(grp);
                }
                views.sync();
            }
        }
    });

    // FEATURE 352476: cancel focus on the panel when clicking outside
    connect(this, &PanelView::activeFocusItemChanged, cont, [this, cont] {
        if (cont->status() == Plasma::Types::AcceptingInputStatus && !activeFocusItem()) {
            // BUG 454729: avoid switching to PassiveStatus in keyboard navigation
            cont->setStatus(Plasma::Types::ActiveStatus);
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
        const int paddingSignal = rootObject->metaObject()->indexOfSignal("bottomPaddingChanged()");
        const int floatingSignal = rootObject->metaObject()->indexOfSignal("floatingnessTargetChanged()");
        const int touchingWindowSignal = rootObject->metaObject()->indexOfSignal("touchingWindowChanged()");

        if (paddingSignal >= 0) {
            connect(rootObject, SIGNAL(bottomPaddingChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(topPaddingChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(rightPaddingChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(leftPaddingChanged()), this, SLOT(updatePadding()));
        }
        if (floatingSignal >= 0) {
            // Floating animation
            m_floatingnessAnimation.setDuration(rootObject->property("floatingnessAnimationDuration").toInt());
            m_floatingnessAnimation.setTargetObject(rootObject);
            m_floatingnessAnimation.setPropertyName(QByteArrayLiteral("floatingness"));
            connect(&m_floatingnessAnimation, &QPropertyAnimation::valueChanged, rootObject, [this](const QVariant &value) {
                if (m_floatingnessAnimation.state() != QAbstractAnimation::State::Running) {
                    return;
                }
                m_floatingness = get<double>(value);
                positionAndResizePanel();
            });
            connect(&m_floatingnessAnimation, &QPropertyAnimation::finished, rootObject, [this]() {
                updateMask();
            });
            connect(rootObject, SIGNAL(minPanelHeightChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(minPanelWidthChanged()), this, SLOT(updatePadding()));
            connect(rootObject, SIGNAL(leftShadowMarginChanged()), this, SLOT(updateShadows()));
            connect(rootObject, SIGNAL(topShadowMarginChanged()), this, SLOT(updateShadows()));
            connect(rootObject, SIGNAL(rightShadowMarginChanged()), this, SLOT(updateShadows()));
            connect(rootObject, SIGNAL(bottomShadowMarginChanged()), this, SLOT(updateShadows()));
            connect(rootObject, SIGNAL(floatingnessAnimationDurationChanged()), this, SLOT(updateFloatingAnimationDuration()));
            connect(rootObject, SIGNAL(floatingnessTargetChanged()), this, SLOT(updateFloating()));
        }

        updateFloating();

        const QVariant maskProperty = rootObject->property("panelMask");
        if (static_cast<QMetaType::Type>(maskProperty.typeId()) == QMetaType::QRegion) {
            connect(rootObject, SIGNAL(panelMaskChanged()), this, SLOT(updateMask()));
            updateMask();
        }
        if (touchingWindowSignal >= 0) {
            connect(rootObject, SIGNAL(touchingWindowChanged()), this, SLOT(updateTouchingWindow()));
        }
        updateTouchingWindow();
    }
}

void PanelView::refreshStatus(Plasma::Types::ItemStatus status)
{
    if (status == Plasma::Types::NeedsAttentionStatus) {
        showTemporarily();
        setFlags(flags() | Qt::WindowDoesNotAcceptFocus);
        if (KWindowSystem::isPlatformX11()) {
            KX11Extras::setState(winId(), NET::SkipSwitcher | NET::KeepAbove);
        }
        if (m_layerWindow) {
            m_layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
            requestUpdate();
        }
    } else if (status == Plasma::Types::AcceptingInputStatus) {
        m_corona->savePreviousWindow();
        setFlags(flags() & ~Qt::WindowDoesNotAcceptFocus);
        if (KWindowSystem::isPlatformX11()) {
            KX11Extras::forceActiveWindow(winId());
        } else {
            showTemporarily();
        }

        if (m_layerWindow) {
            m_layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
            requestUpdate();
        }

        const auto nextItem = rootObject()->nextItemInFocusChain();
        if (nextItem) {
            nextItem->forceActiveFocus(Qt::TabFocusReason);
        } else {
            containment()->setStatus(Plasma::Types::PassiveStatus);
        }
    } else {
        if (status == Plasma::Types::PassiveStatus) {
            m_corona->restorePreviousWindow();
        } else if (status == Plasma::Types::ActiveStatus) {
            m_corona->clearPreviousWindow();
        }

        restoreAutoHide();
        setFlags(flags() | Qt::WindowDoesNotAcceptFocus);
        if (KWindowSystem::isPlatformX11()) {
            KX11Extras::setState(winId(), NET::SkipSwitcher | NET::KeepAbove);
        }
        if (m_layerWindow) {
            m_layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
            requestUpdate();
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

void PanelView::updateTouchingWindow()
{
    if (!rootObject()) {
        return;
    }
    m_touchingWindow = rootObject()->property("touchingWindow").toBool();
    restoreAutoHide();
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

bool PanelView::edgeActivated() const
{
    return m_visibilityMode == PanelView::AutoHide || m_visibilityMode == PanelView::DodgeWindows;
}

void PanelView::updateEnabledBorders()
{
    if (!rootObject()) {
        return;
    }
    KSvg::FrameSvg::EnabledBorders borders = KSvg::FrameSvg::AllBorders;
    if (m_backgroundHints == Plasma::Types::NoBackground) {
        borders = KSvg::FrameSvg::NoBorder;
    } else {
        if (m_floatingness == 0) {
            switch (location()) {
            case Plasma::Types::TopEdge:
                borders &= ~KSvg::FrameSvg::TopBorder;
                break;
            case Plasma::Types::LeftEdge:
                borders &= ~KSvg::FrameSvg::LeftBorder;
                break;
            case Plasma::Types::RightEdge:
                borders &= ~KSvg::FrameSvg::RightBorder;
                break;
            case Plasma::Types::BottomEdge:
                borders &= ~KSvg::FrameSvg::BottomBorder;
                break;
            default:
                break;
            }
        }

        QMargins shadowPadding = {int(std::round(rootObject()->property("leftShadowMargin").toReal())),
                                  int(std::round(rootObject()->property("topShadowMargin").toReal())),
                                  int(std::round(rootObject()->property("rightShadowMargin").toReal())),
                                  int(std::round(rootObject()->property("bottomShadowMargin").toReal()))};

        if (m_screenToFollow) {
            if (x() - shadowPadding.left() <= m_screenToFollow->geometry().x()) {
                borders &= ~KSvg::FrameSvg::LeftBorder;
            }
            if (x() + width() + shadowPadding.right() >= m_screenToFollow->geometry().x() + m_screenToFollow->geometry().width()) {
                borders &= ~KSvg::FrameSvg::RightBorder;
            }
            if (y() - shadowPadding.top() <= m_screenToFollow->geometry().y()) {
                borders &= ~KSvg::FrameSvg::TopBorder;
            }
            if (y() + height() + shadowPadding.bottom() >= m_screenToFollow->geometry().y() + m_screenToFollow->geometry().height()) {
                borders &= ~KSvg::FrameSvg::BottomBorder;
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
    updateEnabledBorders();

    QMargins extraShadowPadding = {int(std::round(rootObject()->property("leftShadowMargin").toReal())),
                                   int(std::round(rootObject()->property("topShadowMargin").toReal())),
                                   int(std::round(rootObject()->property("rightShadowMargin").toReal())),
                                   int(std::round(rootObject()->property("bottomShadowMargin").toReal()))};
    PanelShadows::self()->addWindow(this, enabledBorders(), extraShadowPadding);
}

void PanelView::updateFloating()
{
    if (!rootObject()) {
        return;
    }

    m_floatingnessAnimation.setStartValue(m_floatingness);
    m_floatingnessAnimation.setEndValue(rootObject()->property("floatingnessTarget").toDouble());
    if (m_floatingnessAnimation.startValue().toInt() > m_floatingnessAnimation.endValue().toInt()) {
        m_floatingnessAnimation.setEasingCurve(QEasingCurve::InCubic);
    } else {
        m_floatingnessAnimation.setEasingCurve(QEasingCurve::OutCubic);
    }
    m_leftFloatingPadding = rootObject()->property("fixedLeftFloatingPadding").toInt();
    m_rightFloatingPadding = rootObject()->property("fixedRightFloatingPadding").toInt();
    m_topFloatingPadding = rootObject()->property("fixedTopFloatingPadding").toInt();
    m_bottomFloatingPadding = rootObject()->property("fixedBottomFloatingPadding").toInt();

    positionAndResizePanel();
    updateExclusiveZone();
    updateShadows();

    // positionPanel and updateMask are called by m_floatingnessAnimation
    if (m_floatingnessAnimation.targetObject()) {
        m_floatingnessAnimation.start();
    }
}

void PanelView::updateFloatingAnimationDuration()
{
    m_floatingnessAnimation.setDuration(rootObject()->property("floatingnessAnimationDuration").toInt());
}

#include "moc_panelview.cpp"
