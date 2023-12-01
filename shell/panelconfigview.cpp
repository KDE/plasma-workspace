/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "panelconfigview.h"
#include "config-X11.h"
#include "panelshadows_p.h"
#include "panelview.h"
#include "shellcorona.h"

#include <LayerShellQt/Window>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>

#include <KWindowSystem>
#include <plasmaquick/popupplasmawindow.h>
#include <qnamespace.h>
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
using namespace Qt::StringLiterals;

PanelRulerView::PanelRulerView(Plasma::Containment *containment, PanelView *panelView, PanelConfigView *mainConfigView, QWindow *parent)
    : PlasmaWindow(parent)
    , m_containment(containment)
    , m_panelView(panelView)
    , m_mainConfigView(mainConfigView)
{
    if (KWindowSystem::isPlatformWayland()) {
        m_layerWindow = LayerShellQt::Window::get(this);
        m_layerWindow->setLayer(LayerShellQt::Window::LayerTop);
        m_layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
        m_layerWindow->setScope(QStringLiteral("dock"));
        m_layerWindow->setCloseOnDismissed(false);
    }
    setScreen(m_panelView->screen());

    connect(this, &PanelRulerView::mainItemChanged, this, &PanelRulerView::syncPanelLocation);
}

PanelRulerView::~PanelRulerView()
{
}

void PanelRulerView::syncPanelLocation()
{
    if (!mainItem()) {
        return;
    }
    const QRect available = m_containment->corona()->availableScreenRect(m_containment->screen());

    switch (m_containment->location()) {
    case Plasma::Types::TopEdge:
        setBorders(Qt::BottomEdge);
        break;
    case Plasma::Types::LeftEdge:
        setBorders(Qt::RightEdge);
        break;
    case Plasma::Types::RightEdge:
        setBorders(Qt::LeftEdge);
        break;
    case Plasma::Types::BottomEdge:
    default:
        setBorders(Qt::TopEdge);
    }

    switch (m_containment->location()) {
    case Plasma::Types::LeftEdge:
    case Plasma::Types::RightEdge:
        setMaximumWidth(mainItem()->implicitWidth());
        setWidth(mainItem()->implicitWidth());
        setMaximumHeight(available.height());
        setHeight(available.height());
        break;
    case Plasma::Types::TopEdge:
    case Plasma::Types::BottomEdge:
    default:
        setMaximumWidth(available.width());
        setWidth(available.width());
        setMaximumHeight(mainItem()->implicitHeight());
        setHeight(mainItem()->implicitHeight());
        break;
    }

    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::setType(winId(), NET::Dock);
        KX11Extras::setState(winId(), NET::KeepAbove);
        switch (m_containment->location()) {
        case Plasma::Types::TopEdge:
            setPosition(available.topLeft() + screen()->geometry().topLeft());
            break;
        case Plasma::Types::LeftEdge:
            setPosition(available.topLeft() + screen()->geometry().topLeft());
            break;
        case Plasma::Types::RightEdge:
            setPosition(available.topLeft() + screen()->geometry().topRight() - QPoint(width(), 0));
            break;
        case Plasma::Types::BottomEdge:
        default:
            setPosition(available.bottomLeft() + screen()->geometry().topLeft() - QPoint(0, height()));
        }
    } else if (m_layerWindow) {
        m_layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
        LayerShellQt::Window::Anchors anchors;

        switch (m_containment->location()) {
        case Plasma::Types::TopEdge:
            anchors.setFlag(LayerShellQt::Window::AnchorTop);
            break;
        case Plasma::Types::LeftEdge:
            anchors.setFlag(LayerShellQt::Window::AnchorLeft);
            break;
        case Plasma::Types::RightEdge:
            anchors.setFlag(LayerShellQt::Window::AnchorRight);
            break;
        case Plasma::Types::BottomEdge:
        default:
            anchors.setFlag(LayerShellQt::Window::AnchorBottom);
            break;
        }

        if (m_containment->formFactor() == Plasma::Types::Horizontal) {
            switch (m_panelView->alignment()) {
            case Qt::AlignLeft:
                anchors.setFlag(LayerShellQt::Window::AnchorLeft);
                break;
            case Qt::AlignCenter:
                break;
            case Qt::AlignRight:
                anchors.setFlag(LayerShellQt::Window::AnchorRight);
                break;
            }
        } else {
            switch (m_panelView->alignment()) {
            case Qt::AlignLeft:
                anchors.setFlag(LayerShellQt::Window::AnchorTop);
                break;
            case Qt::AlignCenter:
                break;
            case Qt::AlignRight:
                anchors.setFlag(LayerShellQt::Window::AnchorBottom);
                break;
            }
        }

        m_layerWindow->setAnchors(anchors);

        requestUpdate();
    }
}

void PanelRulerView::showEvent(QShowEvent *ev)
{
    syncPanelLocation();
    PlasmaWindow::showEvent(ev);
}

void PanelRulerView::focusOutEvent(QFocusEvent *ev)
{
    const QWindow *focusWindow = QGuiApplication::focusWindow();

    if (focusWindow
        && ((focusWindow->flags().testFlag(Qt::Popup)) || focusWindow->objectName() == QLatin1String("QMenuClassWindow") || focusWindow == m_mainConfigView)) {
        return;
    }

    m_mainConfigView->hide();
    hide();

    PlasmaWindow::focusOutEvent(ev);
}

//////////////////////////////PanelConfigView
PanelConfigView::PanelConfigView(Plasma::Containment *containment, PanelView *panelView)
    : PlasmaQuick::PopupPlasmaWindow()
    , m_containment(containment)
    , m_panelView(panelView)
    , m_sharedQmlEngine(std::make_unique<PlasmaQuick::SharedQmlEngine>(this))
{
    connect(panelView, &QObject::destroyed, this, &QObject::deleteLater);

    setScreen(panelView->screen());

    connect(panelView, &QWindow::screenChanged, &m_screenSyncTimer, QOverload<>::of(&QTimer::start));
    m_screenSyncTimer.setSingleShot(true);
    m_screenSyncTimer.setInterval(150ms);
    connect(&m_screenSyncTimer, &QTimer::timeout, [this, panelView]() {
        setScreen(panelView->screen());
        syncGeometry();
    });

    m_sharedQmlEngine->rootContext()->setContextProperties({
        QQmlContext::PropertyPair{u"plasmoid"_s, QVariant::fromValue(containment)},
        QQmlContext::PropertyPair{u"panel"_s, QVariant::fromValue(panelView)},
        QQmlContext::PropertyPair{u"configDialog"_s, QVariant::fromValue(this)},
    });
    connect(containment, &Plasma::Containment::formFactorChanged, this, &PanelConfigView::syncGeometry);
    connect(containment, &Plasma::Containment::locationChanged, this, &PanelConfigView::syncGeometry);

    connect(panelView, &PanelView::lengthChanged, this, &PanelConfigView::syncGeometry);
    connect(panelView, &PanelView::geometryChanged, this, &PanelConfigView::syncGeometry);
    connect(panelView, &PanelView::thicknessChanged, this, &PanelConfigView::syncGeometry);
    connect(m_containment->corona(), &Plasma::Corona::editModeChanged, this, [this](bool edit) {
        if (!edit) {
            hide();
        }
    });

    setMargin(4);
}

PanelConfigView::~PanelConfigView()
{
}

void PanelConfigView::init()
{
    m_sharedQmlEngine->setInitializationDelayed(true);
    m_sharedQmlEngine->setSource(m_containment->corona()->kPackage().fileUrl("panelconfigurationui"));
    m_sharedQmlEngine->completeInitialization({{QStringLiteral("panelConfiguration"), QVariant::fromValue(this)}});
    setMainItem(qobject_cast<QQuickItem *>(m_sharedQmlEngine->rootObject()));
    if (mainItem()) {
        if (m_panelRulerView) {
            QQuickItem *ruler = mainItem()->property("panelRuler").value<QQuickItem *>();
            m_panelRulerView->setMainItem(ruler);
            m_panelRulerView->syncPanelLocation();
        }
        setWidth(mainItem()->implicitWidth() + leftPadding() + rightPadding());
        setHeight(mainItem()->implicitHeight() + topPadding() + bottomPadding());
        mainItem()->setVisible(true);
    }
    syncGeometry();
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
    switch (m_containment->location()) {
    case Plasma::Types::TopEdge:
        setPopupDirection(Qt::BottomEdge);
        break;
    case Plasma::Types::LeftEdge:
        setPopupDirection(Qt::RightEdge);
        break;
    case Plasma::Types::RightEdge:
        setPopupDirection(Qt::LeftEdge);
        break;
    case Plasma::Types::BottomEdge:
    default:
        setPopupDirection(Qt::TopEdge);
    }
    queuePositionUpdate();
    update();
    if (m_panelRulerView) {
        m_panelRulerView->syncPanelLocation();
    }
}

void PanelConfigView::keyPressEvent(QKeyEvent *ev)
{
    QQuickWindow::keyPressEvent(ev);
    if (ev->isAccepted()) {
        return;
    }

    if (ev->matches(QKeySequence::Cancel)) {
        ev->accept();
        hide();
    }
}

void PanelConfigView::showEvent(QShowEvent *ev)
{
    if (m_containment) {
        m_containment->setUserConfiguring(true);
    }
    PopupPlasmaWindow::showEvent(ev);
}

void PanelConfigView::hideEvent(QHideEvent *ev)
{
    PopupPlasmaWindow::hideEvent(ev);

    if (m_containment) {
        m_containment->setUserConfiguring(false);
    }
    deleteLater();
}

void PanelConfigView::focusVisibilityCheck(QWindow *focusWindow)
{
    if (!focusWindow) {
        hide();
        return;
    }
    if (focusWindow == this || (m_panelRulerView && focusWindow == m_panelRulerView.get())) {
        return;
    }
    if (auto popup = qobject_cast<const PopupPlasmaWindow *>(focusWindow)) {
        if (auto parent = popup->visualParent(); parent && parent->window() == m_panelView) {
            return;
        }
    }
    hide();
}

void PanelConfigView::focusInEvent(QFocusEvent *ev)
{
    connect(qApp, &QGuiApplication::focusWindowChanged, this, &PanelConfigView::focusVisibilityCheck, Qt::UniqueConnection);
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

PanelRulerView *PanelConfigView::panelRulerView()
{
    if (!m_panelRulerView) {
        m_panelRulerView = std::make_unique<PanelRulerView>(m_containment, m_panelView, this);
        // It's a queued connection because m_panelRulerView needs a bit to have the proper size after visibleChanged is emitted
        connect(
            m_panelRulerView.get(),
            &PanelRulerView::visibleChanged,
            this,
            [this](bool visible) {
                if (visible) {
                    setMargin(std::min(m_panelRulerView->width(), m_panelRulerView->height()) + 4);
                } else {
                    setMargin(4);
                }
            },
            Qt::QueuedConnection);
    }

    if (mainItem()) {
        QQuickItem *ruler = mainItem()->property("panelRuler").value<QQuickItem *>();
        m_panelRulerView->setMainItem(ruler);
        m_panelRulerView->syncPanelLocation();
    }
    return m_panelRulerView.get();
}

#include "moc_panelconfigview.cpp"
