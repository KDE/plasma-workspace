/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "splashapp.h"
#include "splashwindow.h"

#include <QLoggingCategory>
#include <QCommandLineParser>
#include <QCursor>
#include <QDBusConnection>
#include <QPixmap>
#include <qscreen.h>

#include <KWindowSystem>

#include <KConfigGroup>
#include <KSharedConfig>

#include <LayerShellQt/Shell>

Q_LOGGING_CATEGORY(ksplashqml, "org.kde.plasma.ksplashqml", QtWarningMsg)

#define TEST_STEP_INTERVAL 2000

/**
 * There are 7 stages in ksplash
 *  - initial (from this class)
 *  - startPlasma (from startplasma)
 *  - kcminit
 *  - ksmserver
 *  - wm (for X11 from KWin, for Wayland from this class)
 *  - ready (from plasma-session startup)
 *  - desktop (from shellcorona)
 */

SplashApp::SplashApp(int &argc, char **argv)
    : QGuiApplication(argc, argv)
    , m_stage(0)
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
        KConfigGroup ksplashCfg = KSharedConfig::openConfig()->group("KSplash");
        if (ksplashCfg.readEntry("Engine", QStringLiteral("KSplashQML")) == QLatin1String("KSplashQML")) {
            m_theme = ksplashCfg.readEntry("Theme", QStringLiteral("Breeze"));
        }
    }

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/KSplash"), this, QDBusConnection::ExportScriptableSlots);
    dbus.registerService(QStringLiteral("org.kde.KSplash"));

    setupWaylandIntegration();

    foreach (QScreen *screen, screens())
        adoptScreen(screen);

    setStage(QStringLiteral("initial"));

    if (KWindowSystem::isPlatformWayland()) {
        setStage(QStringLiteral("wm"));
    }

    if (m_testing) {
        m_timer.start(TEST_STEP_INTERVAL, this);
    }

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

void SplashApp::setStage(const QString &stage)
{
    qCDebug(ksplashqml) << "Loading stage " << stage << ", current count " << m_stages.count();

    if (m_stages.contains(stage)) {
        return;
    }
    m_stages.append(stage);
    setStage(m_stages.count());
}

void SplashApp::setStage(int stage)
{
    m_stage = stage;
    if (m_stage == 7) {
        QGuiApplication::exit(EXIT_SUCCESS);
    }
    foreach (SplashWindow *w, m_windows) {
        w->setStage(stage);
    }
}

void SplashApp::adoptScreen(QScreen *screen)
{
    if (screen->geometry().isNull()) {
        return;
    }
    SplashWindow *w = new SplashWindow(m_testing, m_window, m_theme, screen);
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
