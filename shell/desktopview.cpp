/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "desktopview.h"
#include "containmentconfigview.h"
#include "shellcorona.h"
#include "shellmanager.h"
#include "krunner_interface.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QScreen>
#include <qopenglshaderprogram.h>

#include <kwindowsystem.h>
#include <klocalizedstring.h>
#include <KAuthorized>

#include <KPackage/Package>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

DesktopView::DesktopView(Plasma::Corona *corona, QScreen *targetScreen)
    : PlasmaQuick::ContainmentView(corona, 0),
      m_windowType(Desktop),
      m_shellSurface(nullptr)
{
    if (targetScreen) {
        setScreenToFollow(targetScreen);
        setScreen(targetScreen);
        setGeometry(targetScreen->geometry());
    }

    setFlags(Qt::Window | Qt::FramelessWindowHint);
    setTitle(corona->kPackage().metadata().name());
    setIcon(QIcon::fromTheme(corona->kPackage().metadata().iconName()));
    rootContext()->setContextProperty(QStringLiteral("desktop"), this);
    setSource(QUrl::fromLocalFile(corona->kPackage().filePath("views", QStringLiteral("Desktop.qml"))));

    connect(this, &QWindow::screenChanged, this, &DesktopView::adaptToScreen);

    QObject::connect(corona, &Plasma::Corona::kPackageChanged,
                     this, &DesktopView::coronaPackageChanged);

    connect(this, &DesktopView::sceneGraphInitialized, this,
        [this, corona]() {
            // check whether the GL Context supports OpenGL
            // Note: hasOpenGLShaderPrograms is broken, see QTBUG--39730
            if (!QOpenGLShaderProgram::hasOpenGLShaderPrograms(openglContext())) {
                qWarning() << "GLSL not available, Plasma won't be functional";
                QMetaObject::invokeMethod(corona, "showOpenGLNotCompatibleWarning", Qt::QueuedConnection);
            }
        }, Qt::DirectConnection);
}

DesktopView::~DesktopView()
{
}

void DesktopView::showEvent(QShowEvent* e)
{
    QQuickWindow::showEvent(e);
    adaptToScreen();
}

void DesktopView::setScreenToFollow(QScreen *screen)
{
    if (screen == m_screenToFollow) {
        return;
    }

    m_screenToFollow = screen;
    setScreen(screen);
    adaptToScreen();
}

QScreen *DesktopView::screenToFollow() const
{
    return m_screenToFollow;
}

void DesktopView::adaptToScreen()
{
    ensureWindowType();

    //This happens sometimes, when shutting down the process
    if (!m_screenToFollow || m_oldScreen==m_screenToFollow) {
        return;
    }

    if(m_oldScreen) {
        disconnect(m_oldScreen.data(), &QScreen::geometryChanged,
                    this, &DesktopView::screenGeometryChanged);
    }
//     qDebug() << "adapting to screen" << m_screenToFollow->name() << this;
    if(m_oldScreen) {
        disconnect(m_oldScreen.data(), &QScreen::geometryChanged,
                    this, &DesktopView::screenGeometryChanged);
    }

    if ((m_windowType == Desktop || m_windowType == WindowedDesktop) && !ShellManager::s_forceWindowed) {
        screenGeometryChanged();

        connect(m_screenToFollow.data(), &QScreen::geometryChanged,
                this, &DesktopView::screenGeometryChanged, Qt::UniqueConnection);
    }

    m_oldScreen = m_screenToFollow;
}

DesktopView::WindowType DesktopView::windowType() const
{
    return m_windowType;
}

void DesktopView::setWindowType(DesktopView::WindowType type)
{
    if (m_windowType == type) {
        return;
    }

    m_windowType = type;

    adaptToScreen();

    emit windowTypeChanged();
}

void DesktopView::ensureWindowType()
{
    //This happens sometimes, when shutting down the process
    if (!screen()) {
        return;
    }

    if (m_windowType == Window || ShellManager::s_forceWindowed) {
        setFlags(Qt::Window);
        KWindowSystem::setType(winId(), NET::Normal);
        KWindowSystem::clearState(winId(), NET::FullScreen);
        if (m_shellSurface) {
            m_shellSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Normal);
            m_shellSurface->setSkipTaskbar(false);
        }

    } else if (m_windowType == Desktop) {
        setFlags(Qt::Window | Qt::FramelessWindowHint);
        KWindowSystem::setType(winId(), NET::Desktop);
        KWindowSystem::setState(winId(), NET::KeepBelow);
        if (m_shellSurface) {
            m_shellSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Desktop);
            m_shellSurface->setSkipTaskbar(true);
        }

    } else if (m_windowType == WindowedDesktop) {
        KWindowSystem::setType(winId(), NET::Normal);
        KWindowSystem::clearState(winId(), NET::FullScreen);
        setFlags(Qt::FramelessWindowHint | flags());
        if (m_shellSurface) {
            m_shellSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Normal);
            m_shellSurface->setSkipTaskbar(false);
        }

    } else if (m_windowType == FullScreen) {
        setFlags(Qt::Window);
        KWindowSystem::setType(winId(), NET::Normal);
        KWindowSystem::setState(winId(), NET::FullScreen);
        if (m_shellSurface) {
            m_shellSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Normal);
            m_shellSurface->setSkipTaskbar(false);
        }
    }
}

DesktopView::SessionType DesktopView::sessionType() const
{
    if (qobject_cast<ShellCorona *>(corona())) {
        return ShellSession;
    } else {
        return ApplicationSession;
    }
}

bool DesktopView::event(QEvent *e)
{
    if (e->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (KWindowSystem::showingDesktop() && ke->key() == Qt::Key_Escape) {
            ShellCorona *c = qobject_cast<ShellCorona *>(corona());
            if (c) {
                KWindowSystem::setShowingDesktop(false);
            }
        }
    } else if (e->type() == QEvent::PlatformSurface) {
        if (auto pe = dynamic_cast<QPlatformSurfaceEvent*>(e)) {
            switch (pe->surfaceEventType()) {
            case QPlatformSurfaceEvent::SurfaceCreated:
                setupWaylandIntegration();
                break;
            case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
                delete m_shellSurface;
                m_shellSurface = nullptr;
                break;
            }
        }
    }

    return PlasmaQuick::ContainmentView::event(e);
}

void DesktopView::keyPressEvent(QKeyEvent *e)
{
    ContainmentView::keyPressEvent(e);

    // When a key is pressed on desktop when nothing else is active forward the key to krunner
    if ((!e->modifiers() || e->modifiers() == Qt::ShiftModifier) && !e->isAccepted()) {
        const QString text = e->text().trimmed();
        if (!text.isEmpty() && text[0].isPrint()) {
            const QString interface(QStringLiteral("org.kde.krunner"));
            if (!KAuthorized::authorize(QStringLiteral("run_command"))) {
                return;
            }
            org::kde::krunner::App krunner(interface, QStringLiteral("/App"), QDBusConnection::sessionBus());
            krunner.query(text);
            e->accept();
        }
    }
}


void DesktopView::showConfigurationInterface(Plasma::Applet *applet)
{
    if (m_configView) {
        if (m_configView->applet() != applet) {
            m_configView->hide();
            m_configView->deleteLater();
        } else {
            m_configView->show();
            m_configView->requestActivate();
            return;
        }
    }

    if (!applet || !applet->containment()) {
        return;
    }

    Plasma::Containment *cont = qobject_cast<Plasma::Containment *>(applet);

    if (cont && cont->isContainment()) {
        m_configView = new ContainmentConfigView(cont);
    } else {
        m_configView = new PlasmaQuick::ConfigView(applet);
    }
    m_configView.data()->init();
    m_configView.data()->setTransientParent(this);
    m_configView.data()->show();
}

void DesktopView::screenGeometryChanged()
{
    const QRect geo = m_screenToFollow->geometry();
//     qDebug() << "newGeometry" << this << geo << geometry();
    setGeometry(geo);
    setMinimumSize(geo.size());
    setMaximumSize(geo.size());
    if (m_shellSurface) {
        m_shellSurface->setPosition(geo.topLeft());
    }
}


void DesktopView::coronaPackageChanged(const KPackage::Package &package)
{
    setContainment(0);
    setSource(QUrl::fromLocalFile(package.filePath("views", QStringLiteral("Desktop.qml"))));
}

void DesktopView::setupWaylandIntegration()
{
    if (m_shellSurface) {
        // already setup
        return;
    }
    if (ShellCorona *c = qobject_cast<ShellCorona*>(corona())) {
        using namespace KWayland::Client;
        PlasmaShell *interface = c->waylandPlasmaShellInterface();
        if (!interface) {
            return;
        }
        Surface *s = Surface::fromWindow(this);
        if (!s) {
            return;
        }
        m_shellSurface = interface->createSurface(s, this);
        m_shellSurface->setPosition(m_screenToFollow->geometry().topLeft());
    }
}
