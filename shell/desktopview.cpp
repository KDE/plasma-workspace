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
#include "krunner_interface.h"
#include "shellcorona.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreen>
#include <qopenglshaderprogram.h>

#include <KAuthorized>
#include <kactivities/controller.h>
#include <klocalizedstring.h>
#include <kwindowsystem.h>

#include <KPackage/Package>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

DesktopView::DesktopView(Plasma::Corona *corona, QScreen *targetScreen)
    : PlasmaQuick::ContainmentView(corona, nullptr)
    , m_windowType(Desktop)
    , m_shellSurface(nullptr)
{
    QObject::setParent(corona);
    if (targetScreen) {
        setScreenToFollow(targetScreen);
        setScreen(targetScreen);
        setGeometry(targetScreen->geometry());
    }

    setFlags(Qt::Window | Qt::FramelessWindowHint);
    setTitle(corona->kPackage().metadata().name());
    rootContext()->setContextProperty(QStringLiteral("desktop"), this);
    setSource(corona->kPackage().fileUrl("views", QStringLiteral("Desktop.qml")));

    connect(this, &QWindow::screenChanged, this, &DesktopView::adaptToScreen);

    QObject::connect(corona, &Plasma::Corona::kPackageChanged, this, &DesktopView::coronaPackageChanged);

    KActivities::Controller *m_activityController = new KActivities::Controller(this);

    QObject::connect(m_activityController, &KActivities::Controller::activityAdded, this, &DesktopView::candidateContainmentsChanged);
    QObject::connect(m_activityController, &KActivities::Controller::activityRemoved, this, &DesktopView::candidateContainmentsChanged);
}

DesktopView::~DesktopView()
{
}

void DesktopView::showEvent(QShowEvent *e)
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

    // This happens sometimes, when shutting down the process
    if (!m_screenToFollow || m_oldScreen == m_screenToFollow) {
        return;
    }

    if (m_oldScreen) {
        disconnect(m_oldScreen.data(), &QScreen::geometryChanged, this, &DesktopView::screenGeometryChanged);
    }
    //     qDebug() << "adapting to screen" << m_screenToFollow->name() << this;
    if (m_oldScreen) {
        disconnect(m_oldScreen.data(), &QScreen::geometryChanged, this, &DesktopView::screenGeometryChanged);
    }

    if (m_windowType == Desktop || m_windowType == WindowedDesktop) {
        screenGeometryChanged();

        connect(m_screenToFollow.data(), &QScreen::geometryChanged, this, &DesktopView::screenGeometryChanged, Qt::UniqueConnection);
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
    // This happens sometimes, when shutting down the process
    if (!screen()) {
        return;
    }

    if (m_windowType == Window) {
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

QVariantMap DesktopView::candidateContainmentsGraphicItems() const
{
    QVariantMap map;
    if (!containment()) {
        return map;
    }

    for (auto cont : corona()->containmentsForScreen(containment()->screen())) {
        map[cont->activity()] = cont->property("_plasma_graphicObject");
    }
    return map;
}

Q_INVOKABLE QString DesktopView::fileFromPackage(const QString &key, const QString &fileName)
{
    return corona()->kPackage().filePath(key.toUtf8(), fileName);
}

bool DesktopView::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface) {
        switch (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceCreated:
            setupWaylandIntegration();
            ensureWindowType();
            break;
        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
            delete m_shellSurface;
            m_shellSurface = nullptr;
            break;
        }
    } else if (e->type() == QEvent::FocusOut) {
        m_krunnerText.clear();
    }

    return PlasmaQuick::ContainmentView::event(e);
}

bool DesktopView::handleKRunnerTextInput(QKeyEvent *e)
{
    // allow only Shift and GroupSwitch modifiers
    if (e->modifiers() & ~Qt::ShiftModifier & ~Qt::GroupSwitchModifier) {
        return false;
    }
    bool krunnerTextChanged = false;
    const QString eventText = e->text();
    for (const QChar ch : eventText) {
        if (!ch.isPrint()) {
            continue;
        }
        if (ch.isSpace() && m_krunnerText.isEmpty()) {
            continue;
        }
        m_krunnerText += ch;
        krunnerTextChanged = true;
    }
    if (krunnerTextChanged) {
        const QString interface(QStringLiteral("org.kde.krunner"));
        if (!KAuthorized::authorize(QStringLiteral("run_command"))) {
            return false;
        }
        org::kde::krunner::App krunner(interface, QStringLiteral("/App"), QDBusConnection::sessionBus());
        krunner.query(m_krunnerText);
        return true;
    }
    return false;
}

void DesktopView::keyPressEvent(QKeyEvent *e)
{
    ContainmentView::keyPressEvent(e);

    if (e->isAccepted()) {
        return;
    }

    if (e->key() == Qt::Key_Escape && KWindowSystem::showingDesktop()) {
        KWindowSystem::setShowingDesktop(false);
        e->accept();
        return;
    }

    // When a key is pressed on desktop when nothing else is active forward the key to krunner
    if (handleKRunnerTextInput(e)) {
        e->accept();
        return;
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

    if (cont && cont->isContainment() && cont->containmentType() == Plasma::Types::DesktopContainment) {
        m_configView = new ContainmentConfigView(cont);
        // if we changed containment with the config open, relaunch the config dialog but for the new containment
        // third arg is used to disconnect when the config closes
        connect(this, &ContainmentView::containmentChanged, m_configView.data(), [this]() {
            showConfigurationInterface(containment());
        });
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
    if (m_shellSurface) {
        m_shellSurface->setPosition(geo.topLeft());
    }
    emit geometryChanged();
}

void DesktopView::coronaPackageChanged(const KPackage::Package &package)
{
    setContainment(nullptr);
    setSource(package.fileUrl("views", QStringLiteral("Desktop.qml")));
}

void DesktopView::setupWaylandIntegration()
{
    if (m_shellSurface) {
        // already setup
        return;
    }
    if (ShellCorona *c = qobject_cast<ShellCorona *>(corona())) {
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
