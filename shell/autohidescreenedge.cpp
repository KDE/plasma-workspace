/*
    SPDX-FileCopyrightText: 2023 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-plasma.h>

#include "autohidescreenedge.h"
#include "debug.h"
#include "panelview.h"

#include "qwayland-kde-screen-edge-v1.h"

#include <KWindowSystem>

#include <QDebug>
#include <QWindow>
#include <QtWaylandClient/QWaylandClientExtension>
#include <QtWaylandClient/QtWaylandClientVersion>
#include <qpa/qplatformwindow_p.h>

#if HAVE_X11
#include <private/qtx11extras_p.h>
#include <xcb/xcb.h>
#endif

class WaylandScreenEdgeManagerV1 : public QWaylandClientExtensionTemplate<WaylandScreenEdgeManagerV1>, public QtWayland::kde_screen_edge_manager_v1
{
    Q_OBJECT

public:
    WaylandScreenEdgeManagerV1()
        : QWaylandClientExtensionTemplate(1)
    {
        initialize();
    }

    ~WaylandScreenEdgeManagerV1() override
    {
        if (isInitialized()) {
            destroy();
        }
    }
};

class WaylandAutoHideScreenEdgeV1 : public QtWayland::kde_auto_hide_screen_edge_v1
{
public:
    WaylandAutoHideScreenEdgeV1(Plasma::Types::Location location, ::kde_auto_hide_screen_edge_v1 *edge)
        : QtWayland::kde_auto_hide_screen_edge_v1(edge)
        , location(location)
    {
    }

    ~WaylandAutoHideScreenEdgeV1() override
    {
        destroy();
    }

    const Plasma::Types::Location location;
};

class WaylandAutoHideScreenEdge : public AutoHideScreenEdge
{
    Q_OBJECT

public:
    WaylandAutoHideScreenEdge(PanelView *view);

    void deactivate() override;
    void activate() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool create();
    void destroy();

    std::unique_ptr<WaylandScreenEdgeManagerV1> m_manager;
    std::unique_ptr<WaylandAutoHideScreenEdgeV1> m_edge;
    bool m_active = false;
};

WaylandAutoHideScreenEdge::WaylandAutoHideScreenEdge(PanelView *view)
    : AutoHideScreenEdge(view)
{
    m_manager = std::make_unique<WaylandScreenEdgeManagerV1>();
    if (!m_manager->isActive()) {
        qCWarning(PLASMASHELL) << m_manager->extensionInterface()->name << "is unsupported by the compositor. Panel autohide won't work correctly";
        return;
    }

    // Create a screen edge only if there's a surface role. For now, there's no better alternative
    // than check if the window is exposed.
    view->installEventFilter(this);
}

bool WaylandAutoHideScreenEdge::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)
    if (event->type() == QEvent::Expose) {
        const auto window = static_cast<QWindow *>(watched);
        if (!m_edge && m_active && window->isExposed()) {
            if (create()) {
                m_edge->activate();
            }
        }
    } else if (event->type() == QEvent::Hide) {
        destroy();
    }
    return false;
}

void WaylandAutoHideScreenEdge::deactivate()
{
    m_active = false;
    if (m_edge) {
        m_edge->deactivate();
    }
}

void WaylandAutoHideScreenEdge::activate()
{
    if (!m_manager->isActive()) {
        return;
    }

    if (m_edge && m_edge->location != m_view->location()) {
        m_edge.reset();
    }

    if (!m_edge) {
        if (m_view->isExposed()) {
            create();
        }
    }

    m_active = true;
    if (m_edge) {
        m_edge->activate();
    }
}

bool WaylandAutoHideScreenEdge::create()
{
    auto waylandWindow = m_view->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
    if (!waylandWindow || !waylandWindow->surface()) {
        return false;
    }

    uint32_t border;
    switch (m_view->location()) {
    case Plasma::Types::Location::LeftEdge:
        border = QtWayland::kde_screen_edge_manager_v1::border_left;
        break;
    case Plasma::Types::Location::RightEdge:
        border = QtWayland::kde_screen_edge_manager_v1::border_right;
        break;
    case Plasma::Types::Location::TopEdge:
        border = QtWayland::kde_screen_edge_manager_v1::border_top;
        break;
    case Plasma::Types::Location::BottomEdge:
    default:
        border = QtWayland::kde_screen_edge_manager_v1::border_bottom;
        break;
    }

    m_edge = std::make_unique<WaylandAutoHideScreenEdgeV1>(m_view->location(), m_manager->get_auto_hide_screen_edge(border, waylandWindow->surface()));
    return true;
}

void WaylandAutoHideScreenEdge::destroy()
{
    m_edge.reset();
}

#if HAVE_X11

class X11AutoHideScreenEdge : public AutoHideScreenEdge
{
    Q_OBJECT

public:
    X11AutoHideScreenEdge(PanelView *view);
    ~X11AutoHideScreenEdge() override;

    void deactivate() override;
    void activate() override;

private:
    xcb_atom_t m_atom = XCB_ATOM_NONE;
};

X11AutoHideScreenEdge::X11AutoHideScreenEdge(PanelView *view)
    : AutoHideScreenEdge(view)
{
    xcb_connection_t *connection = QX11Info::connection();

    const QByteArray atomName = QByteArrayLiteral("_KDE_NET_WM_SCREEN_EDGE_SHOW");
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom_unchecked(connection, false, atomName.length(), atomName.constData());
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    if (reply) {
        m_atom = reply->atom;
        free(reply);
    }
}

X11AutoHideScreenEdge::~X11AutoHideScreenEdge()
{
    if (!m_view) {
        return;
    }
    deactivate();
}

void X11AutoHideScreenEdge::deactivate()
{
    if (m_atom != XCB_ATOM_NONE) {
        xcb_delete_property(QX11Info::connection(), m_view->winId(), m_atom);
    }
}

void X11AutoHideScreenEdge::activate()
{
    if (m_atom == XCB_ATOM_NONE) {
        return;
    }

    uint32_t value = 0;

    switch (m_view->location()) {
    case Plasma::Types::TopEdge:
        value = 0;
        break;
    case Plasma::Types::RightEdge:
        value = 1;
        break;
    case Plasma::Types::BottomEdge:
        value = 2;
        break;
    case Plasma::Types::LeftEdge:
        value = 3;
        break;
    case Plasma::Types::Floating:
    default:
        value = 4;
        break;
    }

    xcb_change_property(QX11Info::connection(), XCB_PROP_MODE_REPLACE, m_view->winId(), m_atom, XCB_ATOM_CARDINAL, 32, 1, &value);
}

#endif

AutoHideScreenEdge::AutoHideScreenEdge(PanelView *view)
    : QObject(view)
    , m_view(view)
{
}

AutoHideScreenEdge *AutoHideScreenEdge::create(PanelView *view)
{
    if (KWindowSystem::isPlatformWayland()) {
        return new WaylandAutoHideScreenEdge(view);
    } else {
#if HAVE_X11
        return new X11AutoHideScreenEdge(view);
#else
        Q_UNREACHABLE();
#endif
    }
}

#include "autohidescreenedge.moc"
