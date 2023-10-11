/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "view.h"

#include <QAction>
#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>
#include <QPlatformSurfaceEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreen>

#include <KAuthorized>
#include <KCrash>
#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KService>
#include <KWindowEffects>
#include <KWindowSystem>
#include <KX11Extras>

#include <LayerShellQt/Window>

#include "appadaptor.h"
#include "x11windowscreenrelativepositioner.h"

KCONFIGGROUP_DECLARE_ENUM_QOBJECT(View, HistoryBehavior)

View::View(QWindow *)
    : PlasmaQuick::PlasmaWindow()
    , m_floating(false)
{
    KCrash::initialize();
    qmlRegisterUncreatableType<View>("org.kde.krunner.private.view", 1, 0, "HistoryBehavior", QStringLiteral("Only for enums"));

    if (KWindowSystem::isPlatformX11()) {
        m_x11Positioner = new X11WindowScreenRelativePositioner(this);
    }

    // used only by screen readers
    setTitle(i18n("KRunner"));

    m_config = KConfigGroup(KSharedConfig::openConfig(), "General");
    m_stateData = KSharedConfig::openConfig(QStringLiteral("krunnerstaterc"), //
                                            KConfig::NoGlobals,
                                            QStandardPaths::GenericDataLocation)
                      ->group("General");
    m_configWatcher = KConfigWatcher::create(KSharedConfig::openConfig());
    connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group) {
        const QLatin1String pluginsGrp("Plugins");
        if (group.name() == QLatin1String("General")) {
            loadConfig();
        } else if (group.name() == pluginsGrp) {
            Q_EMIT helpEnabledChanged();
        } else if (group.name() == QLatin1String("Favorites") && group.parent().name() == pluginsGrp) {
            assignFavoriteIds();
        }
    });

    loadConfig();

    new AppAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/App"), this);

    m_engine = new PlasmaQuick::SharedQmlEngine(this);
    m_engine->engine()->setProperty("_kirigamiTheme", QStringLiteral("KirigamiPlasmaStyle"));
    m_engine->setInitializationDelayed(true);
    connect(m_engine, &PlasmaQuick::SharedQmlEngine::finished, this, &View::objectIncubated);

    m_engine->engine()->rootContext()->setContextProperty(QStringLiteral("runnerWindow"), this);
    m_engine->setSource(QUrl(QStringLiteral("qrc:/krunner/RunCommand.qml")));
    m_engine->completeInitialization();

    auto screenRemoved = [this](QScreen *screen) {
        if (screen == this->screen()) {
            setScreen(qGuiApp->primaryScreen());
            hide();
        }
    };

    connect(qGuiApp, &QGuiApplication::screenRemoved, this, screenRemoved);
    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &View::slotFocusWindowChanged);
}

View::~View()
{
}

void View::objectIncubated()
{
    auto mainItem = qobject_cast<QQuickItem *>(m_engine->rootObject());
    setMainItem(mainItem);
    resize(QSize(mainItem->implicitWidth(), mainItem->implicitHeight()).grownBy(padding()));
    connect(mainItem, &QQuickItem::implicitHeightChanged, this, [mainItem, this]() {
        resize(QSize(mainItem->implicitWidth(), mainItem->implicitHeight()).grownBy(padding()));
    });
}

void View::slotFocusWindowChanged()
{
    if (QGuiApplication::focusWindow() && m_requestedClipboardSelection) {
        displayWithClipboardContents();
    }

    if (!QGuiApplication::focusWindow() && !m_pinned) {
        setVisible(false);
    }
}

bool View::freeFloating() const
{
    return m_floating;
}

void View::setFreeFloating(bool floating)
{
    m_floating = floating;
    if (m_floating) {
        KWindowEffects::slideWindow(this, KWindowEffects::NoEdge);
        setBorders(Qt::Edges());
    } else {
        KWindowEffects::slideWindow(this, KWindowEffects::TopEdge);
        setBorders(Qt::LeftEdge | Qt::RightEdge | Qt::BottomEdge);
    }

    positionOnScreen();
}

void View::loadConfig()
{
    setFreeFloating(m_config.readEntry("FreeFloating", false));
    setRetainPriorSearch(m_config.readEntry("RetainPriorSearch", true));
    setPinned(m_stateData.readEntry("Pinned", false));
    setHistoryBehavior(m_config.readEntry("historyBehavior", m_historyBehavior));
    assignFavoriteIds();
}

void View::showEvent(QShowEvent *event)
{
    KX11Extras::setOnAllDesktops(winId(), true);
    QQuickWindow::showEvent(event);
    requestActivate();
    KX11Extras::forceActiveWindow(winId());
}

void View::positionOnScreen()
{
    const auto screens = QGuiApplication::screens();
    auto screenIt = screens.cend();
    if (KWindowSystem::isPlatformWayland()) {
        auto message = QDBusMessage::createMethodCall("org.kde.KWin", "/KWin", "org.kde.KWin", "activeOutputName");
        QDBusReply<QString> reply = QDBusConnection::sessionBus().call(message);
        if (reply.isValid()) {
            const QString activeOutputName = reply.value();
            screenIt = std::find_if(screens.cbegin(), screens.cend(), [&activeOutputName](QScreen *screen) {
                return screen->name() == activeOutputName;
            });
        }
    } else if (KWindowSystem::isPlatformX11()) {
        screenIt = std::find_if(screens.cbegin(), screens.cend(), [](QScreen *screen) {
            return screen->geometry().contains(QCursor::pos(screen));
        });
    }

    QScreen *const shownOnScreen = screenIt != screens.cend() ? *screenIt : QGuiApplication::primaryScreen();
    setScreen(shownOnScreen);

    QMargins margins;
    if (m_floating) {
        const QRect r = shownOnScreen->availableGeometry();
        margins = QMargins({0, r.height() / 3, 0, 0});
    }

    if (KWindowSystem::isPlatformWayland()) {
        auto layerWindow = LayerShellQt::Window::get(this);
        layerWindow->setAnchors(LayerShellQt::Window::AnchorTop);
        layerWindow->setLayer(LayerShellQt::Window::LayerTop);
        layerWindow->setScope(QStringLiteral("krunner"));
        layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
        layerWindow->setMargins(margins);
    } else if (KWindowSystem::isPlatformX11()) {
        m_x11Positioner->setAnchors(Qt::TopEdge);
        m_x11Positioner->setMargins(margins);
        if (m_floating) {
            KX11Extras::setOnDesktop(winId(), KX11Extras::currentDesktop());
        } else {
            KX11Extras::setOnAllDesktops(winId(), true);
        }
        KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
    }
}

void View::toggleDisplay()
{
    if (isVisible() && !QGuiApplication::focusWindow()) {
        KX11Extras::forceActiveWindow(winId());
        return;
    }
    if (isVisible()) {
        setVisible(false);
    } else {
        display();
    }
}

void View::display()
{
    positionOnScreen();
    setVisible(true);
}

void View::displaySingleRunner(const QString &runnerName)
{
    display();

    m_engine->rootObject()->setProperty("singleRunner", runnerName);
    m_engine->rootObject()->setProperty("query", QString());
}

void View::displayWithClipboardContents()
{
    display();

    // On Wayland we cannot retrieve the clipboard selection until we get the focus
    if (QGuiApplication::focusWindow()) {
        m_requestedClipboardSelection = false;
        m_engine->rootObject()->setProperty("singleRunner", QString());
        m_engine->rootObject()->setProperty("query", QGuiApplication::clipboard()->text(QClipboard::Selection));
    } else {
        m_requestedClipboardSelection = true;
    }
}

void View::query(const QString &term)
{
    display();

    m_engine->rootObject()->setProperty("singleRunner", QString());
    m_engine->rootObject()->setProperty("query", term);
}

void View::querySingleRunner(const QString &runnerName, const QString &term)
{
    display();

    m_engine->rootObject()->setProperty("singleRunner", runnerName);
    m_engine->rootObject()->setProperty("query", term);
}

bool View::pinned() const
{
    return m_pinned;
}

void View::setPinned(bool pinned)
{
    if (m_pinned != pinned) {
        m_pinned = pinned;
        m_stateData.writeEntry("Pinned", pinned);
        Q_EMIT pinnedChanged();
    }
}
