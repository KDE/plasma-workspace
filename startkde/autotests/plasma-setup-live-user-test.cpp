// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include <QProcess>
#include <QTest>

using namespace Qt::StringLiterals;

class PlasmaSetupLiveUserTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
    }

    void runTest()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        qputenv("SYSROOT", dir.path().toUtf8());
        qputenv("HOME", dir.filePath(u"home/foobar/"_s).toUtf8());
        unsetenv("XDG_CONFIG_HOME");

        QDir().mkpath(dir.filePath(u"usr/share/applications"_s));
        QFile f(dir.filePath(u"usr/share/applications/calamares.desktop"_s));
        f.open(QFile::WriteOnly);
        f.close();

        auto executable = QFINDTESTDATA("../bin/plasma-setup-live-user");
        QProcess setup;
        setup.setProgram(executable);
        setup.setProcessChannelMode(QProcess::ForwardedChannels);
        setup.start();
        QVERIFY(setup.waitForFinished());
        QCOMPARE(setup.exitCode(), 0);
        QCOMPARE(setup.exitStatus(), QProcess::NormalExit);

        QVERIFY(QFile::exists(dir.filePath(u"home/foobar/Desktop/calamares.desktop"_s)));
        QVERIFY(QFile::permissions(dir.filePath(u"home/foobar/Desktop/calamares.desktop"_s)) & QFile::ExeOwner);
        QVERIFY(QFile::exists(dir.filePath(u"home/foobar/.config/plasma-welcomerc"_s)));
    }
};

QTEST_GUILESS_MAIN(PlasmaSetupLiveUserTest)

#include "plasma-setup-live-user-test.moc"
