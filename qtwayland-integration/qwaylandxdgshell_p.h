// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Eurogiciel, author: <philippe.coval@eurogiciel.fr>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDXDGSHELL_H
#define QWAYLANDXDGSHELL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qwayland-xdg-shell.h"

#include "qwaylandxdgdecorationv1_p.h"
#include "qwaylandxdgactivationv1_p.h"

#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include <QtCore/QSize>
#include <QtGui/QRegion>

QT_BEGIN_NAMESPACE

class QWindow;

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandInputDevice;
class QWaylandXdgShell;
class QWaylandXdgExportedV2;
class QWaylandXdgExporterV2;

class Q_WAYLANDCLIENT_EXPORT QWaylandXdgSurface : public QWaylandShellSurface, public QtWayland::xdg_surface
{
    Q_OBJECT
public:
    QWaylandXdgSurface(QWaylandXdgShell *shell, ::xdg_surface *surface, QWaylandWindow *window);
    ~QWaylandXdgSurface() override;

    bool resize(QWaylandInputDevice *inputDevice, Qt::Edges edges) override;
    bool move(QWaylandInputDevice *inputDevice) override;
    bool showWindowMenu(QWaylandInputDevice *seat) override;
    void setTitle(const QString &title) override;
    void setAppId(const QString &appId) override;
    void setWindowFlags(Qt::WindowFlags flags) override;

    bool isExposed() const override;
    bool handleExpose(const QRegion &) override;
    bool handlesActiveState() const { return m_toplevel; }
    void applyConfigure() override;
    bool wantsDecorations() const override;
    void propagateSizeHints() override;
    void setWindowGeometry(const QRect &rect) override;
    bool requestActivate() override;
    void setXdgActivationToken(const QString &token) override;
    void requestXdgActivationToken(quint32 serial) override;
    void setAlertState(bool enabled) override;
    bool isAlertState() const override { return m_alertState; }
    QString externWindowHandle() override;

    void setSizeHints();

    void *nativeResource(const QByteArray &resource);

    std::any surfaceRole() const override;

protected:
    void requestWindowStates(Qt::WindowStates states) override;
    void xdg_surface_configure(uint32_t serial) override;

private:
    class Toplevel: public QtWayland::xdg_toplevel
    {
    public:
        Toplevel(QWaylandXdgSurface *xdgSurface);
        ~Toplevel() override;

        void applyConfigure();
        bool wantsDecorations();

        void xdg_toplevel_configure(int32_t width, int32_t height, wl_array *states) override;
        void xdg_toplevel_close() override;
        void xdg_toplevel_configure_bounds(int32_t width, int32_t height) override;

        void requestWindowFlags(Qt::WindowFlags flags);
        void requestWindowStates(Qt::WindowStates states);

        static resize_edge convertToResizeEdges(Qt::Edges edges);

        struct {
            QSize bounds = {0, 0};
            QSize size = {0, 0};
            Qt::WindowStates states = Qt::WindowNoState;
        }  m_pending, m_applied;
        QWaylandWindow::ToplevelWindowTilingStates m_toplevelStates = QWaylandWindow::WindowNoState;
        QSize m_normalSize;

        QWaylandXdgSurface *m_xdgSurface = nullptr;
        QWaylandXdgToplevelDecorationV1 *m_decoration = nullptr;
        QScopedPointer<QWaylandXdgExportedV2> m_exported;
    };

    class Popup : public QtWayland::xdg_popup {
    public:
        Popup(QWaylandXdgSurface *xdgSurface, QWaylandWindow *parent, QtWayland::xdg_positioner *positioner);
        ~Popup() override;

        void applyConfigure();
        void resetConfiguration();

        void grab(QWaylandInputDevice *seat, uint serial);
        void xdg_popup_configure(int32_t x, int32_t y, int32_t width, int32_t height) override;
        void xdg_popup_popup_done() override;

        QWaylandXdgSurface *m_xdgSurface = nullptr;
        QWaylandXdgSurface *m_parentXdgSurface = nullptr;
        QWaylandWindow *m_parent = nullptr;
        bool m_grabbing = false;

        QRect m_pendingGeometry;
    };

    void setToplevel();
    void setPopup(QWaylandWindow *parent);
    void setGrabPopup(QWaylandWindow *parent, QWaylandInputDevice *device, int serial);

    QWaylandXdgShell *m_shell = nullptr;
    QWaylandWindow *m_window = nullptr;
    Toplevel *m_toplevel = nullptr;
    Popup *m_popup = nullptr;
    bool m_configured = false;
    QRegion m_exposeRegion;
    uint m_pendingConfigureSerial = 0;
    uint m_appliedConfigureSerial = 0;
    QString m_activationToken;
    QString m_appId;
    bool m_alertState = false;

    friend class QWaylandXdgShell;
};

class Q_WAYLANDCLIENT_EXPORT QWaylandXdgShell : public QtWayland::xdg_wm_base
{
public:
    QWaylandXdgShell(QWaylandDisplay *display, uint32_t id, uint32_t availableVersion);
    ~QWaylandXdgShell() override;

    QWaylandDisplay *display() const { return m_display; }

    QWaylandXdgDecorationManagerV1 *decorationManager() { return m_xdgDecorationManager.data(); }
    QWaylandXdgActivationV1 *activation() const { return m_xdgActivation.data(); }
    QWaylandXdgExporterV2 *exporter() const { return m_xdgExporter.data(); }
    QWaylandXdgSurface *getXdgSurface(QWaylandWindow *window);

protected:
    void xdg_wm_base_ping(uint32_t serial) override;

private:
    static void handleRegistryGlobal(void *data, ::wl_registry *registry, uint id,
                                     const QString &interface, uint version);

    QWaylandDisplay *m_display = nullptr;
    QScopedPointer<QWaylandXdgDecorationManagerV1> m_xdgDecorationManager;
    QScopedPointer<QWaylandXdgActivationV1> m_xdgActivation;
    QScopedPointer<QWaylandXdgExporterV2> m_xdgExporter;
    QWaylandXdgSurface::Popup *m_topmostGrabbingPopup = nullptr;

    friend class QWaylandXdgSurface;
};

QT_END_NAMESPACE

}

#endif // QWAYLANDXDGSHELL_H
