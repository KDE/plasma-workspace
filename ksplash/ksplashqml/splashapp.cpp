/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "splashapp.h"
#include "splashwindow.h"

#include <QCommandLineParser>
#include <QCursor>
#include <QDBusConnection>
#include <QDate>
#include <QDebug>
#include <QPixmap>
#include <qscreen.h>

#include <KQuickAddons/QtQuickSettings>
#include <KWindowSystem>
#include <LayerShellQt/Shell>

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
    KQuickAddons::QtQuickSettings::init();

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

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/KSplash"), this, QDBusConnection::ExportScriptableSlots);
    dbus.registerService(QStringLiteral("org.kde.KSplash"));

    setupWaylandIntegration();

    Q_FOREACH (QScreen *screen, screens())
        adoptScreen(screen);

    setStage(QStringLiteral("initial"));

    if (KWindowSystem::isPlatformWayland()) {
        setStage(QStringLiteral("wm"));
    }

    QPixmap cursor(32, 32);
    cursor.fill(Qt::transparent);
    setOverrideCursor(QCursor(cursor));

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
    Q_FOREACH (SplashWindow *w, m_windows) {
        w->setStage(stage);
    }
}

void SplashApp::adoptScreen(QScreen *screen)
{
    if (screen->geometry().isNull()) {
        return;
    }
    SplashWindow *w = new SplashWindow(m_testing, m_window, m_theme);
    w->setGeometry(screen->geometry());
    w->setScreen(screen);
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
