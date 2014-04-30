/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
// own
#include <config-ksmserver.h>
// Qt
#include <QDebug>
#include <QtTest/QtTest>
// system
#include <signal.h>

Q_DECLARE_METATYPE(QProcess::ExitStatus)

class KillTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testKill_data();
    void testKill();
    void testImmediateKill_data();
    void testImmediateKill();
};

void KillTest::testKill_data()
{
    QTest::addColumn<int>("signal");
    QTest::addColumn<bool>("expectedQuit");
    QTest::addColumn<QProcess::ExitStatus>("exitStatus");

    QTest::newRow("SIGHUP")      << SIGHUP    << true  << QProcess::CrashExit;
    QTest::newRow("SIGINT")      << SIGINT    << true  << QProcess::CrashExit;
    QTest::newRow("SIGQUIT")     << SIGQUIT   << true  << QProcess::CrashExit;
    QTest::newRow("SIGILL")      << SIGILL    << true  << QProcess::CrashExit;
    QTest::newRow("SIGTRAP")     << SIGTRAP   << true  << QProcess::CrashExit;
    QTest::newRow("SIGABRT")     << SIGABRT   << true  << QProcess::CrashExit;
    QTest::newRow("SIGIOT")      << SIGIOT    << true  << QProcess::CrashExit;
    QTest::newRow("SIGBUS")      << SIGBUS    << true  << QProcess::CrashExit;
    QTest::newRow("SIGFPE")      << SIGFPE    << true  << QProcess::CrashExit;
    QTest::newRow("SIGKILL")     << SIGKILL   << true  << QProcess::CrashExit;
    QTest::newRow("SIGUSR1")     << SIGUSR1   << false << QProcess::CrashExit;
    QTest::newRow("SIGSEGV")     << SIGSEGV   << true  << QProcess::CrashExit;
    QTest::newRow("SIGUSR2")     << SIGUSR2   << true  << QProcess::CrashExit;
    QTest::newRow("SIGPIPE")     << SIGPIPE   << true  << QProcess::CrashExit;
    QTest::newRow("SIGALRM")     << SIGALRM   << true  << QProcess::CrashExit;
    QTest::newRow("SIGTERM")     << SIGTERM   << true  << QProcess::NormalExit;
    QTest::newRow("SIGSTKFLT")   << SIGSTKFLT << true  << QProcess::CrashExit;
    // ignore
    //QTest::newRow("SIGCHLD")   << SIGCHLD;
    //QTest::newRow("SIGCONT")   << SIGCONT;
    //QTest::newRow("SIGSTOP")   << SIGSTOP;
    //QTest::newRow("SIGTSTP")   << SIGTSTP;
    //QTest::newRow("SIGTTIN")   << SIGTTIN;
    //QTest::newRow("SIGTTOU")   << SIGTTOU;
    //QTest::newRow("SIGURG")    << SIGURG;
    //QTest::newRow("SIGXCPU")   << SIGXCPU;
    //QTest::newRow("SIGXFSZ")   << SIGXFSZ;
    //QTest::newRow("SIGVTALRM") << SIGVTALRM;
    //QTest::newRow("SIGPROF")   << SIGPROF;
    //QTest::newRow("SIGWINCH")  << SIGWINCH;
    //QTest::newRow("SIGIO")     << SIGIO;
    //QTest::newRow("SIGPWR")    << SIGPWR;
    QTest::newRow("SIGSYS")      << SIGSYS    << true << QProcess::CrashExit;
    QTest::newRow("SIGUNUSED")   << SIGUNUSED << true << QProcess::CrashExit;
}

void KillTest::testKill()
{
    QProcess greeter(this);
    greeter.setReadChannel(QProcess::StandardOutput);
    greeter.start(QStringLiteral(KSCREENLOCKER_GREET_BIN), QStringList({QStringLiteral("--testing")}));
    QVERIFY(greeter.waitForStarted());

    // wait some time till it's really set up
    QTest::qSleep(5000);

    // now kill
    QFETCH(int, signal);
    kill(greeter.pid(), signal);

    QFETCH(bool, expectedQuit);
    QCOMPARE(greeter.waitForFinished(1000), expectedQuit);
    if (greeter.state() == QProcess::Running) {
        greeter.terminate();
        QVERIFY(greeter.waitForFinished());
    } else {
        QFETCH(QProcess::ExitStatus, exitStatus);
        QCOMPARE(greeter.exitStatus(), exitStatus);

        if (greeter.exitStatus() == QProcess::NormalExit) {
            // exit code is only valid for NormalExit
            QCOMPARE(greeter.exitCode(), 1);
        }
    }
}

void KillTest::testImmediateKill_data()
{
    QTest::addColumn<int>("signal");

    QTest::newRow("SIGHUP")      << SIGHUP;
    QTest::newRow("SIGINT")      << SIGINT;
    QTest::newRow("SIGQUIT")     << SIGQUIT;
    QTest::newRow("SIGILL")      << SIGILL;
    QTest::newRow("SIGTRAP")     << SIGTRAP;
    QTest::newRow("SIGABRT")     << SIGABRT;
    QTest::newRow("SIGIOT")      << SIGIOT;
    QTest::newRow("SIGBUS")      << SIGBUS;
    QTest::newRow("SIGFPE")      << SIGFPE;
    QTest::newRow("SIGKILL")     << SIGKILL;
    QTest::newRow("SIGUSR1")     << SIGUSR1;
    QTest::newRow("SIGSEGV")     << SIGSEGV;
    QTest::newRow("SIGUSR2")     << SIGUSR2;
    QTest::newRow("SIGPIPE")     << SIGPIPE;
    QTest::newRow("SIGALRM")     << SIGALRM;
    QTest::newRow("SIGTERM")     << SIGTERM;
    QTest::newRow("SIGSTKFLT")   << SIGSTKFLT;
    // ignore
    //QTest::newRow("SIGCHLD")   << SIGCHLD;
    //QTest::newRow("SIGCONT")   << SIGCONT;
    //QTest::newRow("SIGSTOP")   << SIGSTOP;
    //QTest::newRow("SIGTSTP")   << SIGTSTP;
    //QTest::newRow("SIGTTIN")   << SIGTTIN;
    //QTest::newRow("SIGTTOU")   << SIGTTOU;
    //QTest::newRow("SIGURG")    << SIGURG;
    //QTest::newRow("SIGXCPU")   << SIGXCPU;
    //QTest::newRow("SIGXFSZ")   << SIGXFSZ;
    //QTest::newRow("SIGVTALRM") << SIGVTALRM;
    //QTest::newRow("SIGPROF")   << SIGPROF;
    //QTest::newRow("SIGWINCH")  << SIGWINCH;
    //QTest::newRow("SIGIO")     << SIGIO;
    //QTest::newRow("SIGPWR")    << SIGPWR;
    QTest::newRow("SIGSYS")      << SIGSYS;
    QTest::newRow("SIGUNUSED")   << SIGUNUSED;
}

void KillTest::testImmediateKill()
{
    // this test ensures that the greeter indicates crashexit when a signal is sent to the greeter
    // before it had time to properly setup
    QProcess greeter(this);
    greeter.start(QStringLiteral(KSCREENLOCKER_GREET_BIN), QStringList({QStringLiteral("--testing")}));
    QVERIFY(greeter.waitForStarted());

    // now kill
    QFETCH(int, signal);
    kill(greeter.pid(), signal);

    QVERIFY(greeter.waitForFinished());
    QCOMPARE(greeter.exitStatus(), QProcess::CrashExit);
}

QTEST_MAIN(KillTest)
#include "killtest.moc"
