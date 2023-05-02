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

View::View(QWindow *)
    : PlasmaQuick::Dialog()
    , m_offset(.5)
    , m_floating(false)
{
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    KCrash::initialize();

    // used only by screen readers
    setTitle(i18n("KRunner"));

    m_config = KConfigGroup(KSharedConfig::openConfig(), "General");
    m_stateData = KSharedConfig::openConfig(QStringLiteral("krunnerstaterc"), //
                                            KConfig::NoGlobals,
                                            QStandardPaths::GenericDataLocation)
                      ->group("General");
    m_configWatcher = KConfigWatcher::create(KSharedConfig::openConfig());
    connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        Q_UNUSED(names);
        if (group.name() == QLatin1String("General")) {
            loadConfig();
        } else if (group.name() == QLatin1String("Plugins")) {
            Q_EMIT helpEnabledChanged();
        }
    });

    loadConfig();

    new AppAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/App"), this);

    m_engine = new PlasmaQuick::SharedQmlEngine(this);
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

    auto screenAdded = [this](const QScreen *screen) {
        connect(screen, &QScreen::geometryChanged, this, &View::screenGeometryChanged);
        screenGeometryChanged();
    };

    const auto screens = QGuiApplication::screens();
    for (QScreen *s : screens) {
        screenAdded(s);
    }
    connect(qGuiApp, &QGuiApplication::screenAdded, this, screenAdded);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, screenRemoved);

    connect(KX11Extras::self(), &KX11Extras::workAreaChanged, this, &View::resetScreenPos);

    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &View::slotFocusWindowChanged);
}

View::~View()
{
}

void View::objectIncubated()
{
    auto mainItem = qobject_cast<QQuickItem *>(m_engine->rootObject());
    connect(mainItem, &QQuickItem::widthChanged, this, &View::resetScreenPos);
    setMainItem(mainItem);
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
    if (m_floating == floating) {
        return;
    }

    m_floating = floating;
    if (m_floating) {
        setLocation(Plasma::Types::Floating);
    } else {
        setLocation(Plasma::Types::TopEdge);
    }

    positionOnScreen();
}

void View::loadConfig()
{
    setFreeFloating(m_config.readEntry("FreeFloating", false));
    setPinned(m_stateData.readEntry("Pinned", false));
}

bool View::event(QEvent *event)
{
    // Bypass Dialog so we don't create a plasmashell surface
    if (event->type() == QEvent::Expose || event->type() == QEvent::PlatformSurface) {
        return QQuickWindow::event(event);
    }
    return Dialog::event(event);
}

void View::resizeEvent(QResizeEvent *event)
{
    if (event->oldSize().width() != event->size().width()) {
        positionOnScreen();
    }
}

void View::showEvent(QShowEvent *event)
{
    KX11Extras::setOnAllDesktops(winId(), true);
    QQuickWindow::showEvent(event);
    positionOnScreen();
    requestActivate();
}

void View::screenGeometryChanged()
{
    if (isVisible()) {
        positionOnScreen();
    }
}

void View::resetScreenPos()
{
    if (isVisible() && !m_floating) {
        positionOnScreen();
    }
}

void View::positionOnScreen()
{
    if (!m_requestedVisible) {
        return;
    }

    QScreen *shownOnScreen = QGuiApplication::primaryScreen();

    auto message = QDBusMessage::createMethodCall("org.kde.KWin", "/KWin", "org.kde.KWin", "activeOutputName");
    QDBusReply<QString> reply = QDBusConnection::sessionBus().call(message);

    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (reply.isValid()) {
            if (screen->name() == reply.value()) {
                shownOnScreen = screen;
                break;
            }
        } else if (screen->geometry().contains(QCursor::pos(screen))) {
            shownOnScreen = screen;
            break;
        }
    }
    setScreen(shownOnScreen);
    const QRect r = shownOnScreen->availableGeometry();

    if (KWindowSystem::isPlatformWayland()) {
        auto layerWindow = LayerShellQt::Window::get(this);
        layerWindow->setAnchors(LayerShellQt::Window::AnchorTop);
        layerWindow->setLayer(LayerShellQt::Window::LayerTop);
        layerWindow->setScope(QStringLiteral("krunner"));
        layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
        if (m_floating) {
            layerWindow->setMargins({0, r.height() / 3, 0, 0});
        } else {
            layerWindow->setMargins(QMargins());
            // Workaround so Dialog gets the borders correct
            auto geom = geometry();
            geom.moveCenter({r.center().x(), 0});
            setGeometry(geom);
        }
    } else {
        if (m_floating && !m_customPos.isNull()) {
            int x = qBound(r.left(), m_customPos.x(), r.right() - width());
            int y = qBound(r.top(), m_customPos.y(), r.bottom() - height());
            setPosition(x, y);
            PlasmaQuick::Dialog::setVisible(true);
            return;
        }

        const int w = width();
        int x = r.left() + (r.width() * m_offset) - (w / 2);

        int y = r.top();
        if (m_floating) {
            y += r.height() / 3;
        }
        x = qBound(r.left(), x, r.right() - width());
        y = qBound(r.top(), y, r.bottom() - height());

        setPosition(x, y);

        if (m_floating) {
            KX11Extras::setOnDesktop(winId(), KX11Extras::currentDesktop());
        } else {
            KX11Extras::setOnAllDesktops(winId(), true);
        }
        KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
    }

    if (m_floating) {
        // Turn the sliding effect off
        setLocation(Plasma::Types::Floating);
    } else {
        setLocation(Plasma::Types::TopEdge);
    }

    PlasmaQuick::Dialog::setVisible(true);
    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::forceActiveWindow(winId());
    }
}

void View::toggleDisplay()
{
    if (isVisible() && !QGuiApplication::focusWindow()) {
        KX11Extras::forceActiveWindow(winId());
        return;
    }
    setVisible(!isVisible());
}

void View::display()
{
    setVisible(true);
}

void View::displaySingleRunner(const QString &runnerName)
{
    setVisible(true);

    m_engine->rootObject()->setProperty("singleRunner", runnerName);
    m_engine->rootObject()->setProperty("query", QString());
}

void View::displayWithClipboardContents()
{
    setVisible(true);

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
    setVisible(true);

    m_engine->rootObject()->setProperty("singleRunner", QString());
    m_engine->rootObject()->setProperty("query", term);
}

void View::querySingleRunner(const QString &runnerName, const QString &term)
{
    setVisible(true);

    m_engine->rootObject()->setProperty("singleRunner", runnerName);
    m_engine->rootObject()->setProperty("query", term);
}

void View::displayConfiguration()
{
    auto job = new KIO::CommandLauncherJob(QStringLiteral("kcmshell5"), {QStringLiteral("plasma/kcms/desktop/kcm_krunnersettings")});
    job->start();
}

void View::setVisible(bool visible)
{
    m_requestedVisible = visible;

    if (visible && !m_floating) {
        positionOnScreen();
    } else {
        PlasmaQuick::Dialog::setVisible(visible);
    }
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
