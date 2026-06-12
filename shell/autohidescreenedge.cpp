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

AutoHideScreenEdge::AutoHideScreenEdge(PanelView *view)
    : QObject(view)
    , m_view(view)
{
}

AutoHideScreenEdge *AutoHideScreenEdge::create(PanelView *view)
{
    return new WaylandAutoHideScreenEdge(view);
}

#include "autohidescreenedge.moc"

#include "moc_autohidescreenedge.cpp"
