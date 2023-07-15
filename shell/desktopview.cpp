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

#include <PlasmaQuick/AppletQuickItem>

#include <KAuthorized>
#include <KStartupInfo>
#include <kactivities/controller.h>
#include <klocalizedstring.h>
#include <kwindowsystem.h>

#include <KPackage/Package>

#include <LayerShellQt/Window>

#include <private/qtx11extras_p.h>

DesktopView::DesktopView(Plasma::Corona *corona, QScreen *targetScreen)
    : PlasmaQuick::ContainmentView(corona, nullptr)
    , m_accentColor(Qt::transparent)
{
    QObject::setParent(corona);

    setColor(Qt::black);
    setFlags(Qt::Window | Qt::FramelessWindowHint);

    if (KWindowSystem::isPlatformWayland()) {
        m_layerWindow = LayerShellQt::Window::get(this);
        m_layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
        m_layerWindow->setExclusiveZone(-1);
        m_layerWindow->setLayer(LayerShellQt::Window::LayerBackground);
        m_layerWindow->setScope(QStringLiteral("desktop"));
        m_layerWindow->setCloseOnDismissed(false);
    } else {
        KWindowSystem::setType(winId(), NET::Desktop);
        KWindowSystem::setState(winId(), NET::KeepBelow);
    }

    if (targetScreen) {
        setScreenToFollow(targetScreen);
    } else {
        setTitle(corona->kPackage().metadata().name());
    }

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
            resetAccentColor();
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
    if (screen == m_screenToFollow) {
        return;
    }

    // layer surfaces can't be moved between outputs, so hide and show the window on a new output
    const bool remap = m_layerWindow && isVisible();
    if (remap) {
        setVisible(false);
    }

    if (m_screenToFollow) {
        disconnect(m_screenToFollow.data(), &QScreen::geometryChanged, this, &DesktopView::screenGeometryChanged);
    }
    m_screenToFollow = screen;
    setScreen(screen);
    connect(m_screenToFollow.data(), &QScreen::geometryChanged, this, &DesktopView::screenGeometryChanged);

    if (remap) {
        setVisible(true);
    }

    QString rectString;
    QDebug(&rectString) << screen->geometry();
    setTitle(QStringLiteral("%1 @ %2").arg(corona()->kPackage().metadata().name()).arg(rectString));
    adaptToScreen();
}

QScreen *DesktopView::screenToFollow() const
{
    return m_screenToFollow;
}

void DesktopView::adaptToScreen()
{
    // This happens sometimes, when shutting down the process
    if (!m_screenToFollow) {
        return;
    }

    screenGeometryChanged();
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
    return m_accentColor.value_or(QColor(Qt::transparent));
}

void DesktopView::setAccentColor(const QColor &accentColor)
{
    if (accentColor == m_accentColor) {
        return;
    }

    m_accentColor = accentColor;
    Q_EMIT accentColorChanged(accentColor);
    if (usedInAccentColor()) {
        Q_EMIT static_cast<ShellCorona *>(corona())->colorChanged(accentColor);
    }

    setAccentColorFromWallpaper(accentColor);
}

void DesktopView::resetAccentColor()
{
    if (!m_accentColor.has_value()) {
        return;
    }

    m_accentColor.reset();
    Q_EMIT accentColorChanged(Qt::transparent);
}

#if PROJECT_VERSION_PATCH >= 80
bool DesktopView::showPreviewBanner() const
{
    static const bool shouldShowPreviewBanner = !KConfigGroup(KSharedConfig::openConfig("kdeglobals"), "General").readEntry("HideDesktopPreviewBanner", false);
    return shouldShowPreviewBanner;
}

QString DesktopView::previewBannerTitle() const
{
    static const QString platformInfo = KWindowSystem::isPlatformWayland() ? QStringLiteral("Wayland") : QStringLiteral("X11");

    if constexpr (PROJECT_VERSION_PATCH == 90) {
        return i18nc("@label %1 version string %2 X11/Wayland", "Plasma %1 %2 Beta", WORKSPACE_VERSION_STRING, platformInfo);
    } else {
        return i18nc("@label %1 version string %2 X11/Wayland", "Plasma %1 %2", WORKSPACE_VERSION_STRING, platformInfo);
    }
}

QString DesktopView::previewBannerText() const
{
    return i18nc("@info:usagetip", "Visit bugs.kde.org to report issues");
}
#endif

QVariantMap DesktopView::candidateContainmentsGraphicItems() const
{
    QVariantMap map;
    if (!containment()) {
        return map;
    }

    for (auto cont : corona()->containmentsForScreen(containment()->screen())) {
        map[cont->activity()] = QVariant::fromValue(PlasmaQuick::AppletQuickItem::itemForApplet(cont));
    }
    return map;
}

Q_INVOKABLE QString DesktopView::fileFromPackage(const QString &key, const QString &fileName)
{
    return corona()->kPackage().filePath(key.toUtf8(), fileName);
}

bool DesktopView::event(QEvent *e)
{
    if (e->type() == QEvent::FocusOut) {
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

    if (cont && cont->isContainment() && cont->containmentType() == Plasma::Containment::Desktop) {
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
    setGeometry(m_screenToFollow->geometry());
    Q_EMIT geometryChanged();
}

void DesktopView::coronaPackageChanged(const KPackage::Package &package)
{
    setContainment(nullptr);
    setSource(package.fileUrl("views", QStringLiteral("Desktop.qml")));
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
