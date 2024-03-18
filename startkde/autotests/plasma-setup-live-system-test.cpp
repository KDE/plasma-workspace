// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include <QProcess>
#include <QTest>

using namespace Qt::StringLiterals;

class PlasmaSetupLiveSystemTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        unsetenv("SYSROOT");
        unsetenv("USERNAME");
        unsetenv("CMDLINE");
    }

    void runTest()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        qputenv("SYSROOT", dir.path().toUtf8());
        qputenv("USERNAME", "foobar"_ba);

        QDir().mkpath(dir.filePath(u"etc/xdg/autostart"_s));
        for (const auto &autostartFile : {u"etc/xdg/autostart/distro-release-notifier.desktop"_s, //
                                          u"etc/xdg/autostart/org.kde.discover.notifier.desktop"_s}) {
            const auto path = dir.filePath(autostartFile);
            QFile f(path);
            f.open(QFile::WriteOnly);
            f.close();
            QVERIFY(QFile::exists(path));
        }

        auto executable = QFINDTESTDATA("../bin/plasma-setup-live-system");
        QProcess setup;
        setup.setProgram(executable);
        setup.setProcessChannelMode(QProcess::ForwardedChannels);
        setup.start();
        QVERIFY(setup.waitForFinished());
        QCOMPARE(setup.exitCode(), 0);
        QCOMPARE(setup.exitStatus(), QProcess::NormalExit);

        QVERIFY(QFile::exists(dir.filePath(u"etc/sddm.conf"_s)));
        QVERIFY(QFile::exists(dir.filePath(u"etc/xdg/kscreenlockerrc"_s)));
        QVERIFY(QFile::exists(dir.filePath(u"etc/xdg/kwalletrc"_s)));
        QVERIFY(QFile::exists(dir.filePath(u"etc/xdg/baloorc"_s)));
        QVERIFY(!QFile::exists(dir.filePath(u"home/"_s) + u"foobar"_s)); // no installer = no desktop dir
        QVERIFY(!QFile::exists(dir.filePath(u"etc/xdg/autostart/distro-release-notifier.desktop"_s)));
        QVERIFY(!QFile::exists(dir.filePath(u"etc/xdg/autostart/org.kde.discover.notifier.desktop"_s)));
    }

    void withCmdlineTest()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        qputenv("SYSROOT", dir.path().toUtf8());
        qputenv("CMDLINE", dir.filePath(u"cmdline"_s).toUtf8());
        {
            QFile cmdlineFile(dir.filePath(u"cmdline"_s));
            QVERIFY(cmdlineFile.open(QFile::WriteOnly));
            cmdlineFile.write("bingo=1 bengo plasma.live.user=rollo");
        }

        auto executable = QFINDTESTDATA("../bin/plasma-setup-live-system");
        QProcess setup;
        setup.setProgram(executable);
        setup.setProcessChannelMode(QProcess::ForwardedChannels);
        setup.start();
        QVERIFY(setup.waitForFinished());
        QCOMPARE(setup.exitCode(), 0);
        QCOMPARE(setup.exitStatus(), QProcess::NormalExit);

        QVERIFY(QFile::exists(dir.filePath(u"etc/sddm.conf"_s)));
        QFile sddm(dir.filePath(u"etc/sddm.conf"_s));
        QVERIFY(sddm.open(QFile::ReadOnly));
        QVERIFY(sddm.readAll().contains("rollo"_ba));
    }
};

QTEST_GUILESS_MAIN(PlasmaSetupLiveSystemTest)

#include "plasma-setup-live-system-test.moc"
