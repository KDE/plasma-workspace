/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "splashapp.h"
#include "debug.h"
#include "splashwindow.h"

#include <QCommandLineParser>
#include <QCursor>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

#include <QLoggingCategory>
#include <QPixmap>
#include <QScreen>

#include <KWindowSystem>

#include <KConfigGroup>
#include <KSharedConfig>

#include <LayerShellQt/Shell>
#include <qdbusconnectioninterface.h>

#define TEST_STEP_INTERVAL 200

/**
 * There are 6 stages in ksplash
 *  - initial (from this class)
 *  - startPlasma (from startplasma)
 *  - kcminit (dropped)
 *  - ksmserver
 *  - wm (for X11 from KWin, for Wayland from this class)
 *  - desktop (from shellcorona)
 */

SplashApp::SplashApp(int &argc, char **argv)
    : QGuiApplication(argc, argv)
    , m_stage(1)
    , m_testing(false)
    , m_window(false)
{
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringLiteral("test"), QStringLiteral("Run in test mode")));
    parser.addOption(QCommandLineOption(QStringLiteral("window"), QStringLiteral("Run in windowed mode")));
    parser.addOption(QCommandLineOption(QStringLiteral("nofork"), QStringLiteral("Don't fork")));
    parser.addOption(QCommandLineOption(QStringLiteral("pid"), QStringLiteral("Print the pid of the child process")));
    parser.addPositionalArgument(QStringLiteral("theme"), QStringLiteral("Path to the theme to test"));
    parser.addHelpOption();

    parser.process(*this);
    m_testing = parser.isSet(QStringLiteral("test"));
    m_window = parser.isSet(QStringLiteral("window"));
    m_theme = parser.positionalArguments().value(0);
    if (m_theme.isEmpty()) {
        KConfigGroup ksplashCfg = KSharedConfig::openConfig()->group(QStringLiteral("KSplash"));
        if (ksplashCfg.readEntry("Engine", QStringLiteral("KSplashQML")) == QLatin1String("KSplashQML")) {
            m_theme = ksplashCfg.readEntry("Theme", QStringLiteral("Breeze"));
        }
    }

    if (!m_testing) {
        QDBusServiceWatcher* watcher = new QDBusServiceWatcher(QStringLiteral("org.kde.PlasmaLoading"), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration, this);
        connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, []() {
            qGuiApp->quit();
        });
        if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.PlasmaLoading"))) {
            qGuiApp->quit();
        }
    }

    setupWaylandIntegration();

    for (const auto screenList{screens()}; QScreen * screen : screenList) {
        adoptScreen(screen);
    }

    m_timer.start(TEST_STEP_INTERVAL, this);

    connect(this, &QGuiApplication::screenAdded, this, &SplashApp::adoptScreen);
}

SplashApp::~SplashApp()
{
    qDeleteAll(m_windows);
}

void SplashApp::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timer.timerId()) {
        m_timer.stop();

        setStage(m_stage + 1);

        m_timer.start(TEST_STEP_INTERVAL, this);
    }
}

void SplashApp::setStage(int stage)
{
    m_stage = stage;
    if (m_stage == 6) {
        QGuiApplication::exit(EXIT_SUCCESS);
    }
    for (SplashWindow *w : std::as_const(m_windows)) {
        w->setStage(stage);
    }
}

void SplashApp::adoptScreen(QScreen *screen)
{
    if (screen->geometry().isNull()) {
        return;
    }
    auto *w = new SplashWindow(m_testing, m_window, m_theme, screen);
    w->setGeometry(screen->geometry());
    w->setStage(m_stage);
    w->setVisible(true);
    m_windows << w;

    connect(screen, &QScreen::geometryChanged, w, &SplashWindow::setGeometry);
    connect(screen, &QObject::destroyed, w, [this, w]() {
        m_windows.removeAll(w);
        w->deleteLater();
    });
}

void SplashApp::setupWaylandIntegration()
{
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }
    LayerShellQt::Shell::useLayerShell();
}

#include "moc_splashapp.cpp"
