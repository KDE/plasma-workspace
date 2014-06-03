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
#include <Plasma/Corona>
#include <Plasma/PluginLoader>

//////////////////////////////PanelConfigView
PanelConfigView::PanelConfigView(Plasma::Containment *containment, PanelView *panelView, QWindow *parent)
    : ConfigView(containment, parent),
      m_containment(containment),
      m_panelView(panelView)
{
    m_deleteTimer.setSingleShot(true);
    m_deleteTimer.setInterval(2*60*1000);
    connect(&m_deleteTimer, &QTimer::timeout, this, &PanelConfigView::deleteLater);

    m_visibilityMode = panelView->visibilityMode();

    setScreen(panelView->screen());
    connect(panelView, &QWindow::screenChanged, this,
            [=](QScreen *screen) {
                setScreen(screen);
                syncGeometry();
            });

    KWindowSystem::setType(winId(), NET::Dock);
    setFlags(Qt::WindowFlags((flags() | Qt::FramelessWindowHint) & (~Qt::WindowDoesNotAcceptFocus)));
    KWindowSystem::forceActiveWindow(winId());

    KWindowEffects::enableBlurBehind(winId(), true);
    updateContrast();
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PanelConfigView::updateContrast);

    engine()->rootContext()->setContextProperty("panel", panelView);
    engine()->rootContext()->setContextProperty("configDialog", this);
    connect(containment, &Plasma::Containment::formFactorChanged,
            this, &PanelConfigView::syncGeometry);

    PanelShadows::self()->addWindow(this);
}

PanelConfigView::~PanelConfigView()
{
    m_panelView->setVisibilityMode(m_visibilityMode);
    PanelShadows::self()->removeWindow(this);
}

void PanelConfigView::init()
{
    setSource(QUrl::fromLocalFile(m_containment->corona()->package().filePath("panelconfigurationui")));
    syncGeometry();
}

void PanelConfigView::updateContrast()
{
    KWindowEffects::enableBackgroundContrast(winId(), m_theme.backgroundContrastEnabled(),
                                                      m_theme.backgroundContrast(),
                                                      m_theme.backgroundIntensity(),
                                                      m_theme.backgroundSaturation());
}

void PanelConfigView::showAddWidgetDialog()
{
    QAction *addWidgetAction = m_containment->actions()->action("add widgets");
    if (addWidgetAction) {
        addWidgetAction->trigger();
    }
}

void PanelConfigView::addPanelSpacer()
{
    m_containment->createApplet("org.kde.plasma.panelspacer");
}

void PanelConfigView::syncGeometry()
{
    if (!m_containment || !rootObject()) {
        return;
    }

    if (m_containment->formFactor() == Plasma::Types::Vertical) {
        resize(rootObject()->implicitWidth(), screen()->size().height());

        if (m_containment->location() == Plasma::Types::LeftEdge) {
            setPosition(m_panelView->geometry().right(), screen()->geometry().top());
        } else if (m_containment->location() == Plasma::Types::RightEdge) {
            setPosition(m_panelView->geometry().left() - width(), screen()->geometry().top());
        }

    } else {
        resize(screen()->size().width(), rootObject()->implicitHeight());

        if (m_containment->location() == Plasma::Types::TopEdge) {
            setPosition(screen()->geometry().left(), m_panelView->geometry().bottom());
        } else if (m_containment->location() == Plasma::Types::BottomEdge) {
            setPosition(screen()->geometry().left(), m_panelView->geometry().top() - height());
        }
    }
}

void PanelConfigView::showEvent(QShowEvent *ev)
{
    QQuickWindow::showEvent(ev);

    KWindowSystem::setType(winId(), NET::Dock);
    setFlags(Qt::WindowFlags((flags() | Qt::FramelessWindowHint) & (~Qt::WindowDoesNotAcceptFocus)));
    KWindowSystem::forceActiveWindow(winId());
    KWindowEffects::enableBlurBehind(winId(), true);
    updateContrast();

    if (m_containment) {
        m_containment->setUserConfiguring(true);
    }

    m_deleteTimer.stop();

    if (m_visibilityMode != PanelView::NormalPanel) {
        m_panelView->setVisibilityMode(PanelView::WindowsGoBelow);
    }
    PanelShadows::self()->addWindow(this);
}

void PanelConfigView::hideEvent(QHideEvent *ev)
{
    QQuickWindow::hideEvent(ev);
    m_deleteTimer.start();
    m_panelView->setVisibilityMode(m_visibilityMode);

    if (m_containment) {
        m_containment->setUserConfiguring(false);
    }
}

void PanelConfigView::focusOutEvent(QFocusEvent *ev)
{
    const QWindow *focusWindow = QGuiApplication::focusWindow();

    if (focusWindow && (focusWindow->flags() & Qt::Popup || focusWindow->objectName() == QLatin1String("QMenuClassWindow"))) {
        return;
    }
    Q_UNUSED(ev)
    close();
}

void PanelConfigView::setVisibilityMode(PanelView::VisibilityMode mode)
{
    if (m_visibilityMode == mode) {
        return;
    }

    m_visibilityMode = mode;
    emit visibilityModeChanged();
}

PanelView::VisibilityMode PanelConfigView::visibilityMode() const
{
    return m_visibilityMode;
}

#include "moc_panelconfigview.cpp"
