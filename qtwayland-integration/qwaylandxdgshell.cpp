// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Eurogiciel, author: <philippe.coval@eurogiciel.fr>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandxdgshell_p.h"

#include "qwaylandxdgexporterv2_p.h"

#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylandinputdevice_p.h>
#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandabstractdecoration_p.h>

#include <QtGui/QGuiApplication>
#include <QtGui/private/qwindow_p.h>

#include <LayerShellQt/window.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandXdgSurface::Toplevel::Toplevel(QWaylandXdgSurface *xdgSurface)
    : QtWayland::xdg_toplevel(xdgSurface->get_toplevel())
    , m_xdgSurface(xdgSurface)
{
    QWindow *window = xdgSurface->window()->window();
    if (auto *decorationManager = m_xdgSurface->m_shell->decorationManager()) {
        if (!(window->flags() & Qt::FramelessWindowHint))
            m_decoration = decorationManager->createToplevelDecoration(object());
    }
    requestWindowStates(window->windowStates());
    requestWindowFlags(window->flags());
}

QWaylandXdgSurface::Toplevel::~Toplevel()
{
    // The protocol spec requires that the decoration object is deleted before xdg_toplevel.
    delete m_decoration;
    m_decoration = nullptr;

    if (isInitialized())
        destroy();
}

void QWaylandXdgSurface::Toplevel::applyConfigure()
{
    if (!(m_applied.states & (Qt::WindowMaximized|Qt::WindowFullScreen)))
        m_normalSize = m_xdgSurface->m_window->windowContentGeometry().size();

    if ((m_pending.states & Qt::WindowActive) && !(m_applied.states & Qt::WindowActive)
        && !m_xdgSurface->m_window->display()->isKeyboardAvailable())
        m_xdgSurface->m_window->display()->handleWindowActivated(m_xdgSurface->m_window);

    if (!(m_pending.states & Qt::WindowActive) && (m_applied.states & Qt::WindowActive)
        && !m_xdgSurface->m_window->display()->isKeyboardAvailable())
        m_xdgSurface->m_window->display()->handleWindowDeactivated(m_xdgSurface->m_window);

    m_xdgSurface->m_window->handleToplevelWindowTilingStatesChanged(m_toplevelStates);
    m_xdgSurface->m_window->handleWindowStatesChanged(m_pending.states);

    // If the width or height is zero, the client should decide the size on its own.
    QSize surfaceSize;

    if (m_pending.size.width() > 0) {
        surfaceSize.setWidth(m_pending.size.width());
    } else {
        if (Q_UNLIKELY(m_pending.states & (Qt::WindowMaximized | Qt::WindowFullScreen))) {
            qCWarning(lcQpaWayland) << "Configure event with maximized or fullscreen state contains invalid width:" << m_pending.size.width();
        } else {
            int width = m_normalSize.width();
            if (!m_pending.bounds.isEmpty())
                width = std::min(width, m_pending.bounds.width());
            surfaceSize.setWidth(width);
        }
    }

    if (m_pending.size.height() > 0) {
        surfaceSize.setHeight(m_pending.size.height());
    } else {
        if (Q_UNLIKELY(m_pending.states & (Qt::WindowMaximized | Qt::WindowFullScreen))) {
            qCWarning(lcQpaWayland) << "Configure event with maximized or fullscreen state contains invalid height:" << m_pending.size.height();
        } else {
            int height = m_normalSize.height();
            if (!m_pending.bounds.isEmpty())
                height = std::min(height, m_pending.bounds.height());
            surfaceSize.setHeight(height);
        }
    }

    if (!surfaceSize.isEmpty())
        m_xdgSurface->m_window->resizeFromApplyConfigure(surfaceSize.grownBy(m_xdgSurface->m_window->windowContentMargins()));

    m_applied = m_pending;
    qCDebug(lcQpaWayland) << "Applied pending xdg_toplevel configure event:" << m_applied.size << m_applied.states;
}

bool QWaylandXdgSurface::Toplevel::wantsDecorations()
{
    if (m_decoration && (m_decoration->pending() == QWaylandXdgToplevelDecorationV1::mode_server_side
                         || !m_decoration->isConfigured()))
        return false;

    return !(m_pending.states & Qt::WindowFullScreen);
}

void QWaylandXdgSurface::Toplevel::xdg_toplevel_configure_bounds(int32_t width, int32_t height)
{
    m_pending.bounds = QSize(width, height);
}

void QWaylandXdgSurface::Toplevel::xdg_toplevel_configure(int32_t width, int32_t height, wl_array *states)
{
    m_pending.size = QSize(width, height);

    auto *xdgStates = static_cast<uint32_t *>(states->data);
    size_t numStates = states->size / sizeof(uint32_t);

    m_pending.states = Qt::WindowNoState;
    m_toplevelStates = QWaylandWindow::WindowNoState;

    for (size_t i = 0; i < numStates; i++) {
        switch (xdgStates[i]) {
        case XDG_TOPLEVEL_STATE_ACTIVATED:
            m_pending.states |= Qt::WindowActive;
            break;
        case XDG_TOPLEVEL_STATE_MAXIMIZED:
            m_pending.states |= Qt::WindowMaximized;
            break;
        case XDG_TOPLEVEL_STATE_FULLSCREEN:
            m_pending.states |= Qt::WindowFullScreen;
            break;
        case XDG_TOPLEVEL_STATE_TILED_LEFT:
            m_toplevelStates |= QWaylandWindow::WindowTiledLeft;
            break;
        case XDG_TOPLEVEL_STATE_TILED_RIGHT:
            m_toplevelStates |= QWaylandWindow::WindowTiledRight;
            break;
        case XDG_TOPLEVEL_STATE_TILED_TOP:
            m_toplevelStates |= QWaylandWindow::WindowTiledTop;
            break;
        case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
            m_toplevelStates |= QWaylandWindow::WindowTiledBottom;
            break;
        default:
            break;
        }
    }
    qCDebug(lcQpaWayland) << "Received xdg_toplevel.configure with" << m_pending.size
                          << "and" << m_pending.states;
}

void QWaylandXdgSurface::Toplevel::xdg_toplevel_close()
{
    m_xdgSurface->m_window->window()->close();
}

void QWaylandXdgSurface::Toplevel::requestWindowFlags(Qt::WindowFlags flags)
{
    if (m_decoration) {
        if (flags & Qt::FramelessWindowHint) {
            delete m_decoration;
            m_decoration = nullptr;
        } else {
            m_decoration->unsetMode();
        }
    }
}

void QWaylandXdgSurface::Toplevel::requestWindowStates(Qt::WindowStates states)
{
    // Re-send what's different from the applied state
    Qt::WindowStates changedStates = m_applied.states ^ states;

    if (changedStates & Qt::WindowMaximized) {
        if (states & Qt::WindowMaximized)
            set_maximized();
        else
            unset_maximized();
    }

    if (changedStates & Qt::WindowFullScreen) {
        if (states & Qt::WindowFullScreen) {
            auto screen = m_xdgSurface->window()->waylandScreen();
            if (screen) {
                set_fullscreen(screen->output());
            }
        } else
            unset_fullscreen();
    }

    // Minimized state is not reported by the protocol, so always send it
    if (states & Qt::WindowMinimized) {
        set_minimized();
        m_xdgSurface->window()->handleWindowStatesChanged(states & ~Qt::WindowMinimized);
    }
}

QtWayland::xdg_toplevel::resize_edge QWaylandXdgSurface::Toplevel::convertToResizeEdges(Qt::Edges edges)
{
    return static_cast<enum resize_edge>(
                ((edges & Qt::TopEdge) ? resize_edge_top : 0)
                | ((edges & Qt::BottomEdge) ? resize_edge_bottom : 0)
                | ((edges & Qt::LeftEdge) ? resize_edge_left : 0)
                | ((edges & Qt::RightEdge) ? resize_edge_right : 0));
}

QWaylandXdgSurface::Popup::Popup(QWaylandXdgSurface *xdgSurface, QWaylandWindow *parent,
                                 QtWayland::xdg_positioner *positioner)
    : m_xdgSurface(xdgSurface)
    , m_parentXdgSurface(qobject_cast<QWaylandXdgSurface *>(parent->shellSurface()))
    , m_parent(parent)
{

    init(xdgSurface->get_popup(m_parentXdgSurface ? m_parentXdgSurface->object() : nullptr, positioner->object()));
    if (m_parent) {
        m_parent->addChildPopup(m_xdgSurface->window());
    }
    if (!m_parentXdgSurface) {
        LayerShellQt::Window::attachPopup(parent->window(), object());
    }
}

QWaylandXdgSurface::Popup::~Popup()
{
    if (isInitialized())
        destroy();

    if (m_parent) {
        m_parent->removeChildPopup(m_xdgSurface->window());
    }

    if (m_grabbing) {
        auto *shell = m_xdgSurface->m_shell;
        Q_ASSERT(shell->m_topmostGrabbingPopup == this);
        shell->m_topmostGrabbingPopup = m_parentXdgSurface ? m_parentXdgSurface->m_popup : nullptr;
        m_grabbing = false;

        // Synthesize Qt enter/leave events for popup
        QWindow *leave = nullptr;
        if (m_xdgSurface && m_xdgSurface->window())
            leave = m_xdgSurface->window()->window();
        QWindowSystemInterface::handleLeaveEvent(leave);

        if (QWindow *enter = QGuiApplication::topLevelAt(QCursor::pos()))
            QWindowSystemInterface::handleEnterEvent(enter, enter->mapFromGlobal(QCursor::pos()), QCursor::pos());
    }
}

void QWaylandXdgSurface::Popup::applyConfigure()
{
    if (m_pendingGeometry.isValid()) {
        QRect geometryWithMargins = m_pendingGeometry.marginsAdded(m_xdgSurface->m_window->windowContentMargins());
        QMargins parentMargins = m_parent->windowContentMargins() - m_parent->clientSideMargins();
        QRect globalGeometry = geometryWithMargins.translated(m_parent->geometry().topLeft() + QPoint(parentMargins.left(), parentMargins.top()));
        m_xdgSurface->setGeometryFromApplyConfigure(globalGeometry.topLeft(), globalGeometry.size());
    }
    resetConfiguration();
}

void QWaylandXdgSurface::Popup::resetConfiguration()
{
    m_pendingGeometry = QRect();
}

void QWaylandXdgSurface::Popup::grab(QWaylandInputDevice *seat, uint serial)
{
    m_xdgSurface->m_shell->m_topmostGrabbingPopup = this;
    xdg_popup::grab(seat->wl_seat(), serial);
    m_grabbing = true;
}

void QWaylandXdgSurface::Popup::xdg_popup_configure(int32_t x, int32_t y, int32_t width, int32_t height)
{
    m_pendingGeometry = QRect(x, y, width, height);
}

void QWaylandXdgSurface::Popup::xdg_popup_popup_done()
{
    m_xdgSurface->m_window->window()->close();
}

QWaylandXdgSurface::QWaylandXdgSurface(QWaylandXdgShell *shell, ::xdg_surface *surface, QWaylandWindow *window)
    : QWaylandShellSurface(window)
    , xdg_surface(surface)
    , m_shell(shell)
    , m_window(window)
{
    QWaylandDisplay *display = window->display();
    Qt::WindowType type = window->window()->type();
    auto *transientParent = window->transientParent();

    if (type == Qt::ToolTip && transientParent) {
        setPopup(transientParent);
    } else if (type == Qt::Popup && transientParent && display->lastInputDevice()) {
        setGrabPopup(transientParent, display->lastInputDevice(), display->lastInputSerial());
    } else {
        setToplevel();
        if (transientParent) {
            auto parentXdgSurface = qobject_cast<QWaylandXdgSurface *>(transientParent->shellSurface());
            if (parentXdgSurface)
                m_toplevel->set_parent(parentXdgSurface->m_toplevel->object());
        }
    }
    setSizeHints();
}

QWaylandXdgSurface::~QWaylandXdgSurface()
{
    if (m_toplevel) {
        delete m_toplevel;
        m_toplevel = nullptr;
    }
    if (m_popup) {
        delete m_popup;
        m_popup = nullptr;
    }
    destroy();
}

bool QWaylandXdgSurface::resize(QWaylandInputDevice *inputDevice, Qt::Edges edges)
{
    if (!m_toplevel || !m_toplevel->isInitialized())
        return false;

    auto resizeEdges = Toplevel::convertToResizeEdges(edges);
    m_toplevel->resize(inputDevice->wl_seat(), inputDevice->serial(), resizeEdges);
    return true;
}

bool QWaylandXdgSurface::move(QWaylandInputDevice *inputDevice)
{
    if (m_toplevel && m_toplevel->isInitialized()) {
        m_toplevel->move(inputDevice->wl_seat(), inputDevice->serial());
        return true;
    }
    return false;
}

bool QWaylandXdgSurface::showWindowMenu(QWaylandInputDevice *seat)
{
    if (m_toplevel && m_toplevel->isInitialized()) {
        QPoint position = seat->pointerSurfacePosition().toPoint();
        m_toplevel->show_window_menu(seat->wl_seat(), seat->serial(), position.x(), position.y());
        return true;
    }
    return false;
}

void QWaylandXdgSurface::setTitle(const QString &title)
{
    if (m_toplevel)
        m_toplevel->set_title(title);
}

void QWaylandXdgSurface::setAppId(const QString &appId)
{
    if (m_toplevel)
        m_toplevel->set_app_id(appId);

    m_appId = appId;
}

void QWaylandXdgSurface::setWindowFlags(Qt::WindowFlags flags)
{
    if (m_toplevel)
        m_toplevel->requestWindowFlags(flags);
}

bool QWaylandXdgSurface::isExposed() const
{
    return m_configured || m_pendingConfigureSerial;
}

bool QWaylandXdgSurface::handleExpose(const QRegion &region)
{
    if (!isExposed() && !region.isEmpty()) {
        m_exposeRegion = region;
        return true;
    }
    return false;
}

void QWaylandXdgSurface::applyConfigure()
{
    // It is a redundant ack_configure, so skipped.
    if (m_pendingConfigureSerial == m_appliedConfigureSerial)
        return;

    if (m_toplevel)
        m_toplevel->applyConfigure();
    if (m_popup)
        m_popup->applyConfigure();
    m_appliedConfigureSerial = m_pendingConfigureSerial;

    m_configured = true;
    ack_configure(m_appliedConfigureSerial);
}

bool QWaylandXdgSurface::wantsDecorations() const
{
    return m_toplevel && m_toplevel->wantsDecorations();
}

void QWaylandXdgSurface::propagateSizeHints()
{
    setSizeHints();

    if (m_toplevel && m_window)
        m_window->commit();
}

void QWaylandXdgSurface::setWindowGeometry(const QRect &rect)
{
    set_window_geometry(rect.x(), rect.y(), rect.width(), rect.height());
}

void QWaylandXdgSurface::setSizeHints()
{
    if (m_toplevel && m_window) {
        const int minWidth = qMax(0, m_window->windowMinimumSize().width());
        const int minHeight = qMax(0, m_window->windowMinimumSize().height());
        m_toplevel->set_min_size(minWidth, minHeight);

        int maxWidth = qMax(minWidth, m_window->windowMaximumSize().width());
        if (maxWidth == QWINDOWSIZE_MAX)
            maxWidth = 0;
        int maxHeight = qMax(minHeight, m_window->windowMaximumSize().height());
        if (maxHeight == QWINDOWSIZE_MAX)
            maxHeight = 0;
        m_toplevel->set_max_size(maxWidth, maxHeight);
    }
}

void *QWaylandXdgSurface::nativeResource(const QByteArray &resource)
{
    QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "xdg_surface")
        return object();
    else if (lowerCaseResource == "xdg_toplevel" && m_toplevel)
        return m_toplevel->object();
    else if (lowerCaseResource == "xdg_popup" && m_popup)
        return m_popup->object();
    return nullptr;
}

std::any QWaylandXdgSurface::surfaceRole() const
{
    if (m_toplevel)
        return m_toplevel->object();
    if (m_popup)
        return m_popup->object();
    return {};
}

void QWaylandXdgSurface::requestWindowStates(Qt::WindowStates states)
{
    if (m_toplevel)
        m_toplevel->requestWindowStates(states);
    else
        qCDebug(lcQpaWayland) << "Ignoring window states requested by non-toplevel zxdg_surface_v6.";
}

void QWaylandXdgSurface::setToplevel()
{
    Q_ASSERT(!m_toplevel && !m_popup);
    m_toplevel = new Toplevel(this);
}

void QWaylandXdgSurface::setPopup(QWaylandWindow *parent)
{
    Q_ASSERT(!m_toplevel && !m_popup);

    auto positioner = new QtWayland::xdg_positioner(m_shell->create_positioner());
    // set_popup expects a position relative to the parent
    QRect windowGeometry = m_window->windowContentGeometry();
    QMargins windowMargins = m_window->windowContentMargins() - m_window->clientSideMargins();
    QMargins parentMargins = parent->windowContentMargins() - parent->clientSideMargins();
    QPoint transientPos = m_window->geometry().topLeft(); // this is absolute
    transientPos += QPoint(windowMargins.left(), windowMargins.top());
    transientPos -= parent->geometry().topLeft();
    transientPos -= QPoint(parentMargins.left(), parentMargins.top());
    positioner->set_anchor_rect(transientPos.x(), transientPos.y(), 1, 1);
    positioner->set_anchor(QtWayland::xdg_positioner::anchor_top_left);
    positioner->set_gravity(QtWayland::xdg_positioner::gravity_bottom_right);
    positioner->set_size(windowGeometry.width(), windowGeometry.height());
    positioner->set_constraint_adjustment(QtWayland::xdg_positioner::constraint_adjustment_slide_x
        | QtWayland::xdg_positioner::constraint_adjustment_slide_y);
    m_popup = new Popup(this, parent, positioner);
    positioner->destroy();

    delete positioner;
}

void QWaylandXdgSurface::setGrabPopup(QWaylandWindow *parent, QWaylandInputDevice *device, int serial)
{
    auto parentXdgSurface = qobject_cast<QWaylandXdgSurface *>(parent->shellSurface());
    auto *top = m_shell->m_topmostGrabbingPopup;

    if (top && top->m_xdgSurface != parentXdgSurface) {
        qCWarning(lcQpaWayland) << "setGrabPopup called with a parent," << parentXdgSurface
                                << "which does not match the current topmost grabbing popup,"
                                << top->m_xdgSurface << "According to the xdg-shell protocol, this"
                                << "is not allowed. The wayland QPA plugin is currently handling"
                                << "it by setting the parent to the topmost grabbing popup."
                                << "Note, however, that this may cause positioning errors and"
                                << "popups closing unxpectedly because xdg-shell mandate that child"
                                << "popups close before parents";
        parent = top->m_xdgSurface->m_window;
    }
    setPopup(parent);
    m_popup->grab(device, serial);

    // Synthesize Qt enter/leave events for popup
    if (!parent)
        return;
    QWindow *current = QGuiApplication::topLevelAt(QCursor::pos());
    QWindow *leave = parent->window();
    if (current != leave)
        return;

    QWindowSystemInterface::handleLeaveEvent(leave);

    QWindow *enter = nullptr;
    if (m_popup && m_popup->m_xdgSurface && m_popup->m_xdgSurface->window())
        enter = m_popup->m_xdgSurface->window()->window();

    if (enter)
        QWindowSystemInterface::handleEnterEvent(enter, enter->mapFromGlobal(QCursor::pos()), QCursor::pos());
}

void QWaylandXdgSurface::xdg_surface_configure(uint32_t serial)
{
    m_pendingConfigureSerial = serial;
    if (!m_configured) {
        // We have to do the initial applyConfigure() immediately, since that is the expose.
        applyConfigure();
        m_exposeRegion = QRegion(QRect(QPoint(), m_window->geometry().size()));
    } else {
        // Later configures are probably resizes, so we have to queue them up for a time when we
        // are not painting to the window.
        m_window->applyConfigureWhenPossible();
    }

    if (!m_exposeRegion.isEmpty()) {
        m_window->handleExpose(m_exposeRegion);
        m_exposeRegion = QRegion();
    }
}

bool QWaylandXdgSurface::requestActivate()
{
    if (auto *activation = m_shell->activation()) {
        if (!m_activationToken.isEmpty()) {
            activation->activate(m_activationToken, window()->wlSurface());
            m_activationToken = {};
            return true;
        } else if (const auto token = qEnvironmentVariable("XDG_ACTIVATION_TOKEN"); !token.isEmpty()) {
            activation->activate(token, window()->wlSurface());
            qunsetenv("XDG_ACTIVATION_TOKEN");
            return true;
        } else {
            const auto focusWindow = QGuiApplication::focusWindow();
            // At least GNOME requires to request the token in order to get the
            // focus stealing prevention indication, so requestXdgActivationToken call
            // is still necessary in that case.
            const auto wlWindow = focusWindow ? static_cast<QWaylandWindow*>(focusWindow->handle()) : m_window;
            if (const auto xdgSurface = qobject_cast<QWaylandXdgSurface *>(wlWindow->shellSurface())) {
                if (const auto seat = wlWindow->display()->lastInputDevice()) {
                    const auto tokenProvider = activation->requestXdgActivationToken(
                            wlWindow->display(), wlWindow->wlSurface(), seat->serial(), xdgSurface->m_appId);
                    connect(tokenProvider, &QWaylandXdgActivationTokenV1::done, this,
                            [this, tokenProvider](const QString &token) {
                                m_shell->activation()->activate(token, window()->wlSurface());
                                tokenProvider->deleteLater();
                            });
                    return true;
                }
            }
        }
    }
    return false;
}

void QWaylandXdgSurface::requestXdgActivationToken(quint32 serial)
{
    if (auto *activation = m_shell->activation()) {
        auto tokenProvider = activation->requestXdgActivationToken(
                m_shell->m_display, m_window->wlSurface(), serial, m_appId);
        connect(tokenProvider, &QWaylandXdgActivationTokenV1::done, this,
                [this, tokenProvider](const QString &token) {
                    Q_EMIT m_window->xdgActivationTokenCreated(token);
                    tokenProvider->deleteLater();
                });
    } else {
        QWaylandShellSurface::requestXdgActivationToken(serial);
    }
}

void QWaylandXdgSurface::setXdgActivationToken(const QString &token)
{
    if (m_shell->activation()) {
        m_activationToken = token;
    } else {
        qCWarning(lcQpaWayland) << "zxdg_activation_v1 not available";
    }
}

void QWaylandXdgSurface::setAlertState(bool enabled)
{
    if (m_alertState == enabled)
        return;

    m_alertState = enabled;

    if (!m_alertState)
        return;

    auto *activation = m_shell->activation();
    if (!activation)
        return;

    const auto tokenProvider = activation->requestXdgActivationToken(
            m_shell->m_display, m_window->wlSurface(), std::nullopt, m_appId);
    connect(tokenProvider, &QWaylandXdgActivationTokenV1::done, this,
            [this, tokenProvider](const QString &token) {
                m_shell->activation()->activate(token, m_window->wlSurface());
                tokenProvider->deleteLater();
            });
}

QString QWaylandXdgSurface::externWindowHandle()
{
    if (!m_toplevel || !m_shell->exporter()) {
        return QString();
    }
    if (!m_toplevel->m_exported) {
        m_toplevel->m_exported.reset(m_shell->exporter()->exportToplevel(m_window->wlSurface()));
        // handle events is sent immediately
        m_shell->display()->forceRoundTrip();
    }
    return m_toplevel->m_exported->handle();
}

QWaylandXdgShell::QWaylandXdgShell(QWaylandDisplay *display, uint32_t id, uint32_t availableVersion)
    : QtWayland::xdg_wm_base(display->wl_registry(), id, qMin(availableVersion, 4u))
    , m_display(display)
{
    display->addRegistryListener(&QWaylandXdgShell::handleRegistryGlobal, this);
}

QWaylandXdgShell::~QWaylandXdgShell()
{
    m_display->removeListener(&QWaylandXdgShell::handleRegistryGlobal, this);
    destroy();
}

QWaylandXdgSurface *QWaylandXdgShell::getXdgSurface(QWaylandWindow *window)
{
    return new QWaylandXdgSurface(this, get_xdg_surface(window->wlSurface()), window);
}

void QWaylandXdgShell::xdg_wm_base_ping(uint32_t serial)
{
    pong(serial);
}

void QWaylandXdgShell::handleRegistryGlobal(void *data, wl_registry *registry, uint id,
                                            const QString &interface, uint version)
{
    QWaylandXdgShell *xdgShell = static_cast<QWaylandXdgShell *>(data);
    if (interface == QLatin1String(QWaylandXdgDecorationManagerV1::interface()->name))
        xdgShell->m_xdgDecorationManager.reset(new QWaylandXdgDecorationManagerV1(registry, id, version));

    if (interface == QLatin1String(QWaylandXdgActivationV1::interface()->name)) {
        xdgShell->m_xdgActivation.reset(new QWaylandXdgActivationV1(registry, id, version));
    }

    if (interface == QLatin1String(QWaylandXdgExporterV2::interface()->name)) {
        xdgShell->m_xdgExporter.reset(new QWaylandXdgExporterV2(registry, id, version));
    }
}

}

QT_END_NAMESPACE

#include "moc_qwaylandxdgshell_p.cpp"
