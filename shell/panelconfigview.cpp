/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "panelconfigview.h"
#include "config-X11.h"
#include "desktopview.h"
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
#include <PlasmaQuick/Dialog>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

#include <chrono>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

PanelRulerView::PanelRulerView(Plasma::Containment *containment, PanelView *panelView, PanelConfigView *mainConfigView)
    : PlasmaWindow()
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

        // m_layerWindow->setMargins(margins);
        m_layerWindow->setAnchors(anchors);

        requestUpdate();
    }
}

void PanelRulerView::showEvent(QShowEvent *ev)
{
    syncPanelLocation();
    PlasmaWindow::showEvent(ev);
}

void PanelRulerView::focusOutEvent(QFocusEvent *)
{
    QWindow *focusWindow = QGuiApplication::focusWindow();

    if (focusWindow
        && ((focusWindow->flags().testFlag(Qt::Popup)) || focusWindow->objectName() == QLatin1String("QMenuClassWindow") || focusWindow == m_mainConfigView)) {
        return;
    }

    m_mainConfigView->focusVisibilityCheck(focusWindow);
}

//////////////////////////////PanelConfigView
PanelConfigView::PanelConfigView(Plasma::Containment *containment, PanelView *panelView)
    : PlasmaQuick::PopupPlasmaWindow()
    , m_containment(containment)
    , m_panelView(panelView)
    , m_sharedQmlEngine(std::make_unique<PlasmaQuick::SharedQmlEngine>(this))
{
    ShellCorona *c = qobject_cast<ShellCorona *>(m_containment->corona());
    setProperty("restrictedPopupGeometry", QVariant(c->availableScreenRect(m_containment->screen())));
    connect(m_containment, &Plasma::Containment::screenChanged, this, [this, c](int screen) {
        setProperty("restrictedPopupGeometry", QVariant(c->availableScreenRect(screen)));
    });

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
    connect(containment, &Plasma::Containment::destroyedChanged, this, &QObject::deleteLater);
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
    m_focusWindow = qApp->focusWindow();
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
        auto syncSize = [this] {
            resize(mainItem()->implicitWidth() + leftPadding() + rightPadding(), mainItem()->implicitHeight() + topPadding() + bottomPadding());
        };
        connect(mainItem(), &QQuickItem::implicitWidthChanged, this, syncSize);
        connect(mainItem(), &QQuickItem::implicitHeightChanged, this, syncSize);
        syncSize();
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
    c->evaluateScript(u"panel = panelById(" + QString::number(m_containment->id())
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

void PanelConfigView::moveEvent(QMoveEvent *ev)
{
    PlasmaQuick::PlasmaWindow::moveEvent(ev);
    Q_EMIT geometryChanged(QRect(ev->pos(), size()));
}

void PanelConfigView::resizeEvent(QResizeEvent *ev)
{
    PlasmaQuick::PlasmaWindow::resizeEvent(ev);
    Q_EMIT geometryChanged(QRect(position(), ev->size()));
}

QRect PanelConfigView::geometry() const
{
    return QRect(position(), size());
}

void PanelConfigView::focusVisibilityCheck(QWindow *focusWindow)
{
    QWindow *oldFocusWindow = m_focusWindow;
    m_focusWindow = focusWindow;

    if (!focusWindow) {
        hide();
        return;
    }
    if (focusWindow == this || (m_panelRulerView && focusWindow == m_panelRulerView.get())) {
        return;
    }

    // Do not hide if the focus is on PopupPlasmaWindows opened by the
    // panel, e.g. the settings popup that appear on applet hover.
    if (auto popup = qobject_cast<const PopupPlasmaWindow *>(focusWindow)) {
        if (auto parent = popup->visualParent(); parent && parent->window() == m_panelView) {
            return;
        }
    }
    // Also do not hide if the focus is on Dialogs opened by the panel, such
    // as the "Show Alternatives" dialog.
    if (auto popup = qobject_cast<const PlasmaQuick::Dialog *>(focusWindow)) {
        if (auto parent = popup->visualParent(); parent && parent->window() == m_panelView) {
            return;
        }
    }
    // When the dialogs that appear when hovering applets in the panel close,
    // the focus will switch back to the desktop. If that happens, we
    // instead want to requestActivate to preserve it.
    auto popup = qobject_cast<const PopupPlasmaWindow *>(oldFocusWindow);
    auto desktop = qobject_cast<const DesktopView *>(focusWindow);
    if (desktop && popup && oldFocusWindow != this) {
        requestActivate();
        return;
    }

    // Never hide when the user is handling desktop widgets, e.g. drag
    // and dropping them around.
    if (auto desktop = qobject_cast<const DesktopView *>(focusWindow)) {
        return;
    }

    // Don't close if the user is directly manipulating the panel
    if (m_panelView->mouseGrabberItem()) {
        return;
    }
    hide();
}

void PanelConfigView::focusInEvent(QFocusEvent *ev)
{
    connect(qApp, &QGuiApplication::focusWindowChanged, this, &PanelConfigView::focusVisibilityCheck, Qt::UniqueConnection);
    PopupPlasmaWindow::focusInEvent(ev);
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

QScreen *PanelConfigView::screenFromWindow(QWindow *window) const
{
    if (!window) {
        return nullptr;
    }

    QScreen *screen = window->screen();
    // BUG 483762: If ownership semantics were not previously explicitly set; e.g. ' if the QObject returned from a Q_INVOKABLE function to JS does not
    // have CppOwnership explicitly set, it will become JavaScriptOwnership owned. To avoid that, you can explicitly set the ownership semantic prior to
    // returning it.
    QQmlEngine::setObjectOwnership(screen, QQmlEngine::CppOwnership);
    return screen;
}

#include "moc_panelconfigview.cpp"
