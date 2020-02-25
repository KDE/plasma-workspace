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
#include "panelshadows_p.h"
#include "shellcorona.h"

#include <QDebug>
#include <QDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>
#include <QScreen>
#include <QAction>

#include <klocalizedstring.h>
#include <kwindoweffects.h>
#include <KActionCollection>
#include <KWindowSystem>

#include <Plasma/Containment>
#include <Plasma/PluginLoader>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

//////////////////////////////PanelConfigView
PanelConfigView::PanelConfigView(Plasma::Containment *containment, PanelView *panelView, QWindow *parent)
    : ConfigView(containment, parent),
      m_containment(containment),
      m_panelView(panelView)
{
    connect(panelView, &QObject::destroyed, this, &QObject::deleteLater);

    setScreen(panelView->screen());

    connect(panelView, &QWindow::screenChanged, &m_screenSyncTimer, QOverload<>::of(&QTimer::start));
    m_screenSyncTimer.setSingleShot(true);
    m_screenSyncTimer.setInterval(150);
    connect(&m_screenSyncTimer, &QTimer::timeout,
            [=]() {
                setScreen(panelView->screen());
                KWindowSystem::setType(winId(), NET::Dock);
                KWindowSystem::setState(winId(), NET::KeepAbove);
                syncGeometry();
                syncLocation();
            });

    KWindowSystem::setType(winId(), NET::Dock);
    KWindowSystem::setState(winId(), NET::KeepAbove);
    KWindowSystem::forceActiveWindow(winId());

    updateBlurBehindAndContrast();
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PanelConfigView::updateBlurBehindAndContrast);

    rootContext()->setContextProperty(QStringLiteral("panel"), panelView);
    rootContext()->setContextProperty(QStringLiteral("configDialog"), this);
    connect(containment, &Plasma::Containment::formFactorChanged, this, &PanelConfigView::syncGeometry);
    connect(containment, &Plasma::Containment::locationChanged, this, &PanelConfigView::syncLocation);
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
    KWindowEffects::enableBlurBehind(winId(), m_theme.blurBehindEnabled());
    KWindowEffects::enableBackgroundContrast(winId(), m_theme.backgroundContrastEnabled(),
                                                      m_theme.backgroundContrast(),
                                                      m_theme.backgroundIntensity(),
                                                      m_theme.backgroundSaturation());
}

void PanelConfigView::showAddWidgetDialog()
{
    QAction *addWidgetAction = m_containment->actions()->action(QStringLiteral("add widgets"));
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

    if (m_containment->formFactor() == Plasma::Types::Vertical) {
        QSize s(rootObject()->implicitWidth(), screen()->size().height());
        resize(s);
        setMinimumSize(s);
        setMaximumSize(s);

        if (m_containment->location() == Plasma::Types::LeftEdge) {
            setPosition(m_panelView->geometry().right(), screen()->geometry().top());
        } else if (m_containment->location() == Plasma::Types::RightEdge) {
            setPosition(m_panelView->geometry().left() - width(), screen()->geometry().top());
        }

    } else {
        QSize s(screen()->size().width(), rootObject()->implicitHeight());
        resize(s);
        setMinimumSize(s);
        setMaximumSize(s);

        if (m_containment->location() == Plasma::Types::TopEdge) {
            setPosition(screen()->geometry().left(), m_panelView->geometry().bottom());
        } else if (m_containment->location() == Plasma::Types::BottomEdge) {
            setPosition(screen()->geometry().left(), m_panelView->geometry().top() - height());
        }
    }
}

void PanelConfigView::syncLocation()
{
    if (!m_containment) {
        return;
    }

    KWindowEffects::SlideFromLocation slideLocation = KWindowEffects::NoEdge;
    Plasma::FrameSvg::EnabledBorders enabledBorders = Plasma::FrameSvg::AllBorders;

    switch (m_containment->location()) {
    case Plasma::Types::TopEdge:
        slideLocation = KWindowEffects::TopEdge;
        enabledBorders = Plasma::FrameSvg::BottomBorder;
        break;
    case Plasma::Types::RightEdge:
        slideLocation = KWindowEffects::RightEdge;
        enabledBorders = Plasma::FrameSvg::LeftBorder;
        break;
    case Plasma::Types::BottomEdge:
        slideLocation = KWindowEffects::BottomEdge;
        enabledBorders = Plasma::FrameSvg::TopBorder;
        break;
    case Plasma::Types::LeftEdge:
        slideLocation = KWindowEffects::LeftEdge;
        enabledBorders = Plasma::FrameSvg::RightBorder;
        break;
    default:
        break;
    }

    KWindowEffects::slideWindow(winId(), slideLocation, -1);

    if (m_enabledBorders != enabledBorders) {
        m_enabledBorders = enabledBorders;

        PanelShadows::self()->setEnabledBorders(this, enabledBorders);

        emit enabledBordersChanged();
    }
}

void PanelConfigView::showEvent(QShowEvent *ev)
{
    QQuickWindow::showEvent(ev);

    KWindowSystem::setType(winId(), NET::Dock);
    setFlags(Qt::WindowFlags((flags() | Qt::FramelessWindowHint) & (~Qt::WindowDoesNotAcceptFocus)));
    KWindowSystem::setState(winId(), NET::KeepAbove);
    KWindowSystem::forceActiveWindow(winId());
    updateBlurBehindAndContrast();
    syncGeometry();
    syncLocation();

    //this because due to Qt xcb implementation the actual flags gets set only after a while after the window is actually visible
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
        switch (static_cast<QPlatformSurfaceEvent*>(e)->surfaceEventType()) {
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
    emit visibilityModeChanged();
}

PanelView::VisibilityMode PanelConfigView::visibilityMode() const
{
    return m_panelView->visibilityMode();
}

Plasma::FrameSvg::EnabledBorders PanelConfigView::enabledBorders() const
{
    return m_enabledBorders;
}

#include "moc_panelconfigview.cpp"
