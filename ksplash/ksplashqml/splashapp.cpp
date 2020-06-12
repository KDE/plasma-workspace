/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *   Copyright (C) 2013 Martin Klapetek <mklapetek(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "splashwindow.h"
#include "splashapp.h"

#include <QPixmap>
#include <QCursor>
#include <qscreen.h>
#include <QDBusConnection>
#include <QDate>
#include <QDebug>
#include <QCommandLineParser>

#include <KQuickAddons/QtQuickSettings>
#include <KWindowSystem>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>

#define TEST_STEP_INTERVAL 2000

/**
 * There are 7 used stages in ksplash
 *  - initial
 *  - kcminit
 *  - kinit
 *  - ksmserver
 *  - wm
 *  - ready
 *  - desktop
 */

SplashApp::SplashApp(int &argc, char ** argv)
    : QGuiApplication(argc, argv),
      m_stage(0),
      m_testing(false),
      m_window(false)
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

    foreach(QScreen* screen, screens())
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

void SplashApp::timerEvent(QTimerEvent * event)
{
    if (event->timerId() == m_timer.timerId()) {
        m_timer.stop();

        setStage(m_stage + 1);

        m_timer.start(TEST_STEP_INTERVAL, this);
    }
}

void SplashApp::setStage(const QString &stage)
{
    //filter out startup events from KDED as they will be removed in a future release
    if (stage == QLatin1String("kded") || stage == QLatin1String("confupdate")) {
        return;
    }

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

void SplashApp::adoptScreen(QScreen* screen)
{
    SplashWindow *w = new SplashWindow(m_testing, m_window, m_theme);
    w->setGeometry(screen->geometry());
    w->setStage(m_stage);
    w->setVisible(true);
    m_windows << w;

    connect(screen, &QScreen::geometryChanged, w, &SplashWindow::setGeometry);
    connect(screen, &QObject::destroyed, w, [this, w](){
        m_windows.removeAll(w);
        w->deleteLater();
    });
}

void SplashApp::setupWaylandIntegration()
{
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }
    using namespace KWayland::Client;
    ConnectionThread *connection = ConnectionThread::fromApplication(this);
    if (!connection) {
        return;
    }
    Registry *registry = new Registry(this);
    registry->create(connection);
    connect(registry, &Registry::plasmaShellAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_waylandPlasmaShell = registry->createPlasmaShell(name, version, this);
        }
    );
    registry->setup();
    connection->roundtrip();
}
