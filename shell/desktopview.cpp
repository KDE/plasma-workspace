/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "desktopview.h"
#include "containmentconfigview.h"
#include "krunner_interface.h"
#include "shellcorona.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreen>
#include <qopenglshaderprogram.h>

#include <KAuthorized>
#include <KStartupInfo>
#include <kactivities/controller.h>
#include <klocalizedstring.h>
#include <kwindowsystem.h>

#include <KPackage/Package>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

DesktopView::DesktopView(Plasma::Corona *corona, QScreen *targetScreen)
    : PlasmaQuick::ContainmentView(corona, nullptr)
    , m_accentColor(Qt::transparent)
    , m_windowType(Desktop)
    , m_shellSurface(nullptr)
{
    QObject::setParent(corona);

    // Setting clear color to black makes the panel lose alpha channel on X11. This looks like
    // a QtXCB bug, so set clear color only on Wayland to let the compositor optimize rendering.
    if (KWindowSystem::isPlatformWayland()) {
        setColor(Qt::black);
    }

    if (targetScreen) {
        setScreenToFollow(targetScreen);
    }

    setFlags(Qt::Window | Qt::FramelessWindowHint);
    rootContext()->setContextProperty(QStringLiteral("desktop"), this);
    setSource(corona->kPackage().fileUrl("views", QStringLiteral("Desktop.qml")));
    connect(this, &ContainmentView::containmentChanged, this, &DesktopView::slotContainmentChanged);

    QObject::connect(corona, &Plasma::Corona::kPackageChanged, this, &DesktopView::coronaPackageChanged);

    KActivities::Controller *m_activityController = new KActivities::Controller(this);

    QObject::connect(m_activityController, &KActivities::Controller::activityAdded, this, &DesktopView::candidateContainmentsChanged);
    QObject::connect(m_activityController, &KActivities::Controller::activityRemoved, this, &DesktopView::candidateContainmentsChanged);

    // KRunner settings
    KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("krunnerrc"));
    KConfigGroup configGroup(config, "General");
    m_activateKRunnerWhenTypingOnDesktop = configGroup.readEntry("ActivateWhenTypingOnDesktop", true);

    m_configWatcher = KConfigWatcher::create(config);
    connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (names.contains(QByteArray("ActivateWhenTypingOnDesktop"))) {
            m_activateKRunnerWhenTypingOnDesktop = group.readEntry("ActivateWhenTypingOnDesktop", true);
        }
    });

    // Accent color setting
    connect(static_cast<ShellCorona *>(corona), &ShellCorona::accentColorFromWallpaperEnabledChanged, this, &DesktopView::usedInAccentColorChanged);
    connect(this, &DesktopView::usedInAccentColorChanged, this, [this] {
        if (!usedInAccentColor()) {
            m_accentColor = Qt::transparent;
            Q_EMIT accentColorChanged(m_accentColor);
        }
    });
    connect(this, &ContainmentView::containmentChanged, this, &DesktopView::slotContainmentChanged);
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
    Q_ASSERT(screen);
    qWarning() << "DesktopView::setScreenToFollow" << screen->name() << screen->geometry();
    if (screen == m_screenToFollow) {
        return;
    }

    setTitle("Desktop view for " + screen->name());

    if (m_screenToFollow) {
        disconnect(m_screenToFollow.data(), &QScreen::geometryChanged, this, &DesktopView::screenGeometryChanged);
    }
    m_screenToFollow = screen;
    setScreen(screen);
    connect(m_screenToFollow.data(), &QScreen::geometryChanged, this, &DesktopView::screenGeometryChanged);

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
    if (!m_screenToFollow) {
        return;
    }

    if (m_windowType == Desktop || m_windowType == WindowedDesktop) {
        screenGeometryChanged();
    }
}

bool DesktopView::usedInAccentColor() const
{
    if (!m_containment) {
        return false;
    }

    const bool notPrimaryDisplay = m_containment->screen() != 0;
    if (notPrimaryDisplay) {
        return false;
    }

    return static_cast<ShellCorona *>(corona())->accentColorFromWallpaperEnabled();
}

QColor DesktopView::accentColor() const
{
    return m_accentColor;
}

void DesktopView::setAccentColor(const QColor &accentColor)
{
    if (accentColor == m_accentColor) {
        return;
    }

    m_accentColor = accentColor;
    Q_EMIT accentColorChanged(m_accentColor);
    if (usedInAccentColor()) {
        Q_EMIT static_cast<ShellCorona *>(corona())->colorChanged(m_accentColor);
    }

    setAccentColorFromWallpaper(m_accentColor);
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

    Q_EMIT windowTypeChanged();
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

    if (!m_activateKRunnerWhenTypingOnDesktop) {
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
            auto window = qobject_cast<QWindow *>(m_configView);
            if (window && QX11Info::isPlatformX11()) {
                KStartupInfo::setNewStartupId(window, QX11Info::nextStartupId());
            }
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
    m_configView->init();
    m_configView->setTransientParent(this);
    m_configView->show();
    m_configView->requestActivate();

    auto window = qobject_cast<QWindow *>(m_configView);
    if (window && QX11Info::isPlatformX11()) {
        KStartupInfo::setNewStartupId(window, QX11Info::nextStartupId());
    }
    m_configView->requestActivate();
}

void DesktopView::slotContainmentChanged()
{
    if (m_containment) {
        disconnect(m_containment, &Plasma::Containment::screenChanged, this, &DesktopView::slotScreenChanged);
    }

    m_containment = containment();

    if (m_containment) {
        connect(m_containment, &Plasma::Containment::screenChanged, this, &DesktopView::slotScreenChanged);
        slotScreenChanged(m_containment->screen());
    }
}

void DesktopView::slotScreenChanged(int newId)
{
    if (m_containmentScreenId == newId) {
        return;
    }

    m_containmentScreenId = newId;
    Q_EMIT usedInAccentColorChanged();
}

void DesktopView::screenGeometryChanged()
{
    const QRect geo = m_screenToFollow->geometry();
    //     qDebug() << "newGeometry" << this << geo << geometry();
    setGeometry(geo);
    if (m_shellSurface) {
        m_shellSurface->setPosition(geo.topLeft());
    }
    Q_EMIT geometryChanged();
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

void DesktopView::setAccentColorFromWallpaper(const QColor &accentColor)
{
    if (!usedInAccentColor()) {
        return;
    }
    QDBusMessage applyAccentColor = QDBusMessage::createMethodCall("org.kde.plasmashell.accentColor", "/AccentColor", "", "setAccentColor");
    applyAccentColor << accentColor.rgba();
    QDBusConnection::sessionBus().send(applyAccentColor);
}
