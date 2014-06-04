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

#include "SplashWindow.h"
#include "SplashApp.h"

#include <QDesktopWidget>
#include <QPixmap>
#include <QCursor>
#include <QDBusConnection>
#include <QDateTime>
#include <QDate>
#include <QDebug>
#include <QCommandLineParser>

#define TEST_STEP_INTERVAL 2000

/**
 * There are 7 stages in ksplash
 *  - initial
 *  - kded
 *  - confupdate
 *  - ksmserver
 *  - wm
 *  - ready
 *  - desktop
 */

SplashApp::SplashApp(int &argc, char ** argv)
    : QApplication(argc, argv),
      m_stage(0),
      m_testing(false),
      m_window(false),
      m_startTime(QDateTime::currentDateTime())
{
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption("test", "Run in test mode"));
    parser.addOption(QCommandLineOption("window", "Run in windowed mode"));
    parser.addOption(QCommandLineOption("nofork", "Don't fork"));
    parser.addHelpOption();

    parser.process(*this);
    m_testing = parser.isSet("test");
    m_window = parser.isSet("window");

    m_desktop = QApplication::desktop();
    screenGeometryChanged(m_desktop->screenCount());

    setStage("initial");

    QPixmap cursor(32, 32);
    cursor.fill(Qt::transparent);
    setOverrideCursor(QCursor(cursor));

    if (m_testing) {
        m_timer.start(TEST_STEP_INTERVAL, this);
    }

    connect(m_desktop, SIGNAL(screenCountChanged(int)), this, SLOT(screenGeometryChanged(int)));
    connect(m_desktop, SIGNAL(workAreaResized(int)), this, SLOT(screenGeometryChanged(int)));

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/KSplash"), this, QDBusConnection::ExportScriptableSlots);
    dbus.registerService(QStringLiteral("org.kde.KSplash"));

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
    qDebug() << "Loading stage " << stage << m_startTime.msecsTo(QDateTime::currentDateTime());

    if (m_stages.contains(stage)) {
        return;
    }
    m_stages.append(stage);
    setStage(m_stages.count());
}

void SplashApp::setStage(int stage)
{
    if (m_stage == 7) {
        QApplication::exit(EXIT_SUCCESS);
    }

    m_stage = stage;
    foreach (SplashWindow *w, m_windows) {
        w->setStage(stage);
    }
}

void SplashApp::screenGeometryChanged(int)
{
    int i;
    // first iterate over all the new and old ones to set sizes appropriately
    for (i = 0; i < m_desktop->screenCount(); i++) {
        if (i < m_windows.count()) {
            m_windows[i]->setGeometry(m_desktop->availableGeometry(i));
        }
        else {
            SplashWindow *w = new SplashWindow(m_testing, m_window);
            w->setGeometry(m_desktop->availableGeometry(i));
            w->setStage(m_stage);
            w->show();
            m_windows << w;
        }
    }
    // then delete the rest, if there is any
    m_windows.erase(m_windows.begin() + i, m_windows.end());
}

#include "SplashApp.moc"
