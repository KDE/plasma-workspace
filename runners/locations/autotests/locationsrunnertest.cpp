/*
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KApplicationTrader>
#include <KProtocolInfo>
#include <KRunner/AbstractRunnerTest>
#include <KShell>
#include <QMimeData>
#include <QTest>

class LocationsRunnerTest : public KRunner::AbstractRunnerTest
{
    Q_OBJECT
private:
    QString normalHomeFile;
    QString executableHomeFile;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase()
    {
        QFile::remove(normalHomeFile);
        QFile::remove(executableHomeFile);
    }
    void shouldNotProduceResult();
    void shouldNotProduceResult_data();
    void shouldProduceResult();
    void shouldProduceResult_data();
    void testMimeData();
};

void LocationsRunnerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    // Set up applications.menu so that kservice works
    const QString menusDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String{"/menus"};
    QDir(menusDir).removeRecursively();
    QDir(menusDir).mkpath(QStringLiteral("."));
    QFile::copy(QFINDTESTDATA("../../../menu/desktop/plasma-applications.menu"), menusDir + QLatin1String("/applications.menu"));

    initProperties();
    normalHomeFile = KShell::tildeExpand(QStringLiteral("~/.krunner_locationsrunner_testfile"));
    executableHomeFile = KShell::tildeExpand(QStringLiteral("~/.krunner_locationsrunner_testexecutablefile"));
    QFile normalFile(normalHomeFile);
    normalFile.open(QIODevice::WriteOnly);
    QFile executableFile(executableHomeFile);
    executableFile.open(QIODevice::WriteOnly);
    executableFile.setPermissions(executableFile.permissions() | QFile::ExeOwner);
    QVERIFY(!normalHomeFile.isEmpty());
}

void LocationsRunnerTest::shouldNotProduceResult()
{
    QFETCH(QString, query);
    const auto matches = launchQuery(query);
    QVERIFY(matches.isEmpty());
}

void LocationsRunnerTest::shouldProduceResult()
{
    QFETCH(QString, query);
    QFETCH(QUrl, data);
    const auto matches = launchQuery(query);
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.first().data().toUrl(), data);
}

void LocationsRunnerTest::shouldNotProduceResult_data()
{
    QTest::addColumn<QString>("query");

    QTest::newRow("executable name") << "ls";
    QTest::newRow("executable file path") << "/bin/ls";
    if (!executableHomeFile.isEmpty()) {
        QTest::newRow("executable file in home dir") << executableHomeFile;
    }
    QTest::newRow("executable path and argument") << "/bin/ls -Al";
    QTest::newRow("non existent file") << QDir::homePath() + "_thisfiledoesnotexist.abc";
    QTest::newRow("non existent file URL") << QUrl::fromLocalFile(QDir::homePath() + "_thisfiledoesnotexist.abc").toString();
    QTest::newRow("nonexistent file with $HOME as env variable") << "$HOME/_thisfiledoesnotexist.abc";
    QTest::newRow("nonexistent protocol") << "thisprotocoldoesnotexist:test123";
    QTest::newRow("empty string") << "";
    QTest::newRow("missing closing double quote") << "\"";
    QTest::newRow("missing closing single quote") << "'";
}

void LocationsRunnerTest::shouldProduceResult_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<QUrl>("data");

    const QUrl homeURL = QUrl::fromLocalFile(QDir::homePath());
    QTest::newRow("folder") << QDir::homePath() << homeURL;
    QTest::newRow("folder tilde") << KShell::tildeCollapse(QDir::homePath()) << homeURL;
    QTest::newRow("folder URL") << homeURL.toString() << homeURL;

    QTest::newRow("file") << normalHomeFile << QUrl::fromLocalFile(normalHomeFile);
    QTest::newRow("file tilde") << KShell::tildeCollapse(normalHomeFile) << QUrl::fromLocalFile(normalHomeFile);
    QTest::newRow("file with $HOME as env variable") << KShell::tildeCollapse(normalHomeFile).replace("~", "$HOME") << QUrl::fromLocalFile(normalHomeFile);
    QTest::newRow("file URL") << QUrl::fromLocalFile(normalHomeFile).toString() << QUrl::fromLocalFile(normalHomeFile);
    QTest::newRow("file URL to executable") << QUrl::fromLocalFile(executableHomeFile).toString() << QUrl::fromLocalFile(executableHomeFile);
    if (KProtocolInfo::isHelperProtocol("vnc")) {
        QTest::newRow("vnc URL") << "vnc:foo" << QUrl("vnc:foo");
    }
    if (KApplicationTrader::preferredService("x-scheme-handler/rtmp")) {
        QTest::newRow("rtmp URL") << "rtmp:foo" << QUrl("rtmp:foo");
    }
    if (KApplicationTrader::preferredService("x-scheme-handler/mailto")) {
        // The mailto protocol is not provided by KIO, but by installed apps. BUG: 416257
        QTest::newRow("mailto URL") << "mailto:user.user@user.com" << QUrl("mailto:user.user@user.com");
    }

    if (KProtocolInfo::isKnownProtocol(QStringLiteral("smb"))) {
        QTest::newRow("ssh URL") << "ssh:localhost" << QUrl("ssh:localhost");
        QTest::newRow("help URL") << "help:krunner" << QUrl("help:krunner");
        QTest::newRow("smb URL") << "smb:server/path" << QUrl("smb:server/path");
        QTest::newRow("smb URL shorthand syntax") << R"(\\server\path)" << QUrl("smb://server/path");
    }
}

void LocationsRunnerTest::testMimeData()
{
    const auto matches = launchQuery(QDir::homePath());
    QVERIFY(!matches.isEmpty());
    QMimeData *data = manager->mimeDataForMatch(matches.first());
    QVERIFY(data);
    QCOMPARE(data->urls(), QList<QUrl>{QUrl::fromLocalFile(QDir::homePath())});
}

QTEST_MAIN(LocationsRunnerTest)

#include "locationsrunnertest.moc"
