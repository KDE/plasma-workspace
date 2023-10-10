/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "panelconfigview.h"
#include "config-X11.h"
#include "panelshadows_p.h"
#include "panelview.h"
#include "shellcorona.h"

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>

#include <KWindowSystem>
#if HAVE_X11
#include <KX11Extras>
#endif
#include <klocalizedstring.h>
#include <kwindoweffects.h>

#include <Plasma/Containment>
#include <Plasma/PluginLoader>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

#include <chrono>

using namespace std::chrono_literals;

//////////////////////////////PanelConfigView
PanelConfigView::PanelConfigView(Plasma::Containment *containment, PanelView *panelView, QWindow *parent)
    : ConfigView(containment, parent)
    , m_containment(containment)
    , m_panelView(panelView)
{
    connect(panelView, &QObject::destroyed, this, &QObject::deleteLater);
    engine()->setProperty("_kirigamiTheme", QStringLiteral("KirigamiPlasmaStyle"));

    setScreen(panelView->screen());

    connect(panelView, &QWindow::screenChanged, &m_screenSyncTimer, QOverload<>::of(&QTimer::start));
    m_screenSyncTimer.setSingleShot(true);
    m_screenSyncTimer.setInterval(150ms);
    connect(&m_screenSyncTimer, &QTimer::timeout, [this, panelView]() {
        setScreen(panelView->screen());
#if HAVE_X11
        KX11Extras::setType(winId(), NET::Dock);
#endif
        KWindowSystem::setState(winId(), NET::KeepAbove);
        syncGeometry();
        syncLocation();
    });

#if HAVE_X11
    KX11Extras::setType(winId(), NET::Dock);
    KX11Extras::forceActiveWindow(winId());
#endif
    KWindowSystem::setState(winId(), NET::KeepAbove);

    updateBlurBehindAndContrast();
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PanelConfigView::updateBlurBehindAndContrast);

    rootContext()->setContextProperty(QStringLiteral("panel"), panelView);
    rootContext()->setContextProperty(QStringLiteral("configDialog"), this);
    connect(containment, &Plasma::Containment::formFactorChanged, this, &PanelConfigView::syncGeometry);
    connect(containment, &Plasma::Containment::locationChanged, this, &PanelConfigView::syncGeometry);
    connect(containment, &Plasma::Containment::locationChanged, this, &PanelConfigView::syncLocation);

    connect(panelView, &PanelView::lengthChanged, this, &PanelConfigView::syncGeometry);
    connect(panelView, &PanelView::geometryChanged, this, &PanelConfigView::syncGeometry);
    connect(panelView, &PanelView::thicknessChanged, this, &PanelConfigView::syncGeometry);
}

PanelConfigView::~PanelConfigView()
{
}

void PanelConfigView::init()
{
    setSource(m_containment->corona()->kPackage().fileUrl("panelconfigurationui"));
    syncGeometry();
    syncLocation();
}

void PanelConfigView::updateBlurBehindAndContrast()
{
    KWindowEffects::enableBlurBehind(this, m_theme.blurBehindEnabled());
    KWindowEffects::enableBackgroundContrast(this,
                                             m_theme.backgroundContrastEnabled(),
                                             m_theme.backgroundContrast(),
                                             m_theme.backgroundIntensity(),
                                             m_theme.backgroundSaturation());
}

void PanelConfigView::showAddWidgetDialog()
{
    QAction *addWidgetAction = m_containment->internalAction(QStringLiteral("add widgets"));
    if (addWidgetAction) {
        addWidgetAction->trigger();
    }
}

void PanelConfigView::addPanelSpacer()
{
    ShellCorona *c = qobject_cast<ShellCorona *>(m_containment->corona());
    if (!c) {
        return;
    }
    // Add a spacer at the end *except* if there is exactly one spacer already
    // this to trigger the panel centering mode of the spacer in a slightly more discoverable way
    c->evaluateScript(QStringLiteral("panel = panelById(") + QString::number(m_containment->id())
                      + QStringLiteral(");"
                                       "var spacers = panel.widgets(\"org.kde.plasma.panelspacer\");"
                                       "if (spacers.length === 1) {"
                                       "    panel.addWidget(\"org.kde.plasma.panelspacer\", 0,0,1,1);"
                                       "} else {"
                                       "    panel.addWidget(\"org.kde.plasma.panelspacer\");"
                                       "}"));
}

void PanelConfigView::syncGeometry()
{
    if (!m_containment || !rootObject()) {
        return;
    }

    QRect r(0, 0, rootObject()->implicitHeight(), rootObject()->implicitWidth());
    QRect t = m_panelView->geometry();
    r.moveCenter(t.center());
    if (m_containment->formFactor() == Plasma::Types::Vertical) {
        if (m_containment->location() == Plasma::Types::LeftEdge) {
            r.moveLeft(t.right() + 32);
        } else if (m_containment->location() == Plasma::Types::RightEdge) {
            r.moveRight(t.left() - 32);
        }
    } else {
        if (m_containment->location() == Plasma::Types::TopEdge) {
            r.moveTop(t.bottom() + 32);
        } else if (m_containment->location() == Plasma::Types::BottomEdge) {
            r.moveBottom(t.top() - 32);
        }
    }
    setPosition(r.topLeft());
    resize(r.size());
    setMinimumSize(r.size());
    setMaximumSize(r.size());
}

void PanelConfigView::syncLocation()
{
    if (!m_containment) {
        return;
    }

    KWindowEffects::SlideFromLocation slideLocation = KWindowEffects::NoEdge;
    KSvg::FrameSvg::EnabledBorders enabledBorders = KSvg::FrameSvg::AllBorders;

    switch (m_containment->location()) {
    case Plasma::Types::TopEdge:
        slideLocation = KWindowEffects::TopEdge;
        enabledBorders = KSvg::FrameSvg::BottomBorder;
        break;
    case Plasma::Types::RightEdge:
        slideLocation = KWindowEffects::RightEdge;
        enabledBorders = KSvg::FrameSvg::LeftBorder;
        break;
    case Plasma::Types::BottomEdge:
        slideLocation = KWindowEffects::BottomEdge;
        enabledBorders = KSvg::FrameSvg::TopBorder;
        break;
    case Plasma::Types::LeftEdge:
        slideLocation = KWindowEffects::LeftEdge;
        enabledBorders = KSvg::FrameSvg::RightBorder;
        break;
    default:
        break;
    }

    KWindowEffects::slideWindow(this, slideLocation, -1);

    if (m_enabledBorders != enabledBorders) {
        m_enabledBorders = enabledBorders;

        PanelShadows::self()->setEnabledBorders(this, enabledBorders);

        Q_EMIT enabledBordersChanged();
    }
}

void PanelConfigView::showEvent(QShowEvent *ev)
{
    QQuickWindow::showEvent(ev);

#if HAVE_X11
    KX11Extras::setType(winId(), NET::Dock);
#endif
    setFlags(Qt::WindowFlags((flags() | Qt::FramelessWindowHint) & (~Qt::WindowDoesNotAcceptFocus)) | Qt::X11BypassWindowManagerHint
             | Qt::WindowStaysOnTopHint);
    KWindowSystem::setState(winId(), NET::KeepAbove);
#if HAVE_X11
    KX11Extras::forceActiveWindow(winId());
#endif
    updateBlurBehindAndContrast();
    syncGeometry();
    syncLocation();

    // this because due to Qt xcb implementation the actual flags gets set only after a while after the window is actually visible
    m_screenSyncTimer.start();

    if (m_containment) {
        m_containment->setUserConfiguring(true);
    }

    PanelShadows::self()->addWindow(this, m_enabledBorders);
}

void PanelConfigView::hideEvent(QHideEvent *ev)
{
    QQuickWindow::hideEvent(ev);

    if (m_containment) {
        m_containment->setUserConfiguring(false);
    }
    deleteLater();
}

void PanelConfigView::focusOutEvent(QFocusEvent *ev)
{
    const QWindow *focusWindow = QGuiApplication::focusWindow();

    if (focusWindow && ((focusWindow->flags().testFlag(Qt::Popup)) || focusWindow->objectName() == QLatin1String("QMenuClassWindow"))) {
        return;
    }
    Q_UNUSED(ev)
    close();
}

void PanelConfigView::moveEvent(QMoveEvent *ev)
{
    if (m_shellSurface) {
        m_shellSurface->setPosition(ev->pos());
    }
}

bool PanelConfigView::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface) {
        switch (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceCreated:
            KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager);

            if (m_shellSurface) {
                break;
            }
            if (ShellCorona *c = qobject_cast<ShellCorona *>(m_containment->corona())) {
                using namespace KWayland::Client;
                PlasmaShell *interface = c->waylandPlasmaShellInterface();
                if (!interface) {
                    break;
                }
                Surface *s = Surface::fromWindow(this);
                if (!s) {
                    break;
                }
                m_shellSurface = interface->createSurface(s, this);
                m_shellSurface->setPanelTakesFocus(true);
                m_shellSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Panel);
            }
            break;
        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
            delete m_shellSurface;
            m_shellSurface = nullptr;
            PanelShadows::self()->removeWindow(this);
            break;
        }
    }

    return PlasmaQuick::ConfigView::event(e);
}

void PanelConfigView::setVisibilityMode(PanelView::VisibilityMode mode)
{
    m_panelView->setVisibilityMode(mode);
    Q_EMIT visibilityModeChanged();
}

PanelView::VisibilityMode PanelConfigView::visibilityMode() const
{
    return m_panelView->visibilityMode();
}

void PanelConfigView::setOpacityMode(PanelView::OpacityMode mode)
{
    m_panelView->setOpacityMode(mode);
    Q_EMIT opacityModeChanged();
}

PanelView::OpacityMode PanelConfigView::opacityMode() const
{
    return m_panelView->opacityMode();
}

KSvg::FrameSvg::EnabledBorders PanelConfigView::enabledBorders() const
{
    return m_enabledBorders;
}

#include "moc_panelconfigview.cpp"
