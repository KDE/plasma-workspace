/********************************************************************
Copyright 2016  Eike Hein <hein@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include <QObject>

#include <KConfigGroup>
#include <KDesktopFile>
#include <KIconTheme>
#include <KService>
#include <KSharedConfig>
#include <KSycoca>

#include <QDir>
#include <QIcon>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "tasktools.h"

// Taken from tst_qstandardpaths.
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_BLACKBERRY) && !defined(Q_OS_ANDROID)
#define Q_XDG_PLATFORM
#endif

using namespace TaskManager;

class TaskToolsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void shouldFindApp();
    void shouldFindDefaultApp();
    void shouldCompareLauncherUrls();

private:
    QString appLinkPath();
    void fillReferenceAppData();
    void createAppLink();
    void createIcon();

    AppData m_referenceAppData;
    QTemporaryDir m_tempDir;
};

void TaskToolsTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    QVERIFY(m_tempDir.isValid());
    QVERIFY(QDir().mkpath(m_tempDir.path() + QLatin1String("/config")));
    QVERIFY(QDir().mkpath(m_tempDir.path() + QLatin1String("/cache")));
    QVERIFY(QDir().mkpath(m_tempDir.path() + QLatin1String("/data/applications")));

#ifdef Q_XDG_PLATFORM
    qputenv("XDG_CONFIG_HOME", QFile::encodeName(m_tempDir.path() + QLatin1String("/config")));
    qputenv("XDG_CACHE_HOME", QFile::encodeName(m_tempDir.path() + QLatin1String("/cache")));
    qputenv("XDG_DATA_DIRS", QFile::encodeName(m_tempDir.path() + QLatin1String("/data")));
#else
    QSKIP("This test requires XDG.");
#endif

    createIcon();
    fillReferenceAppData();
    createAppLink();

    QFile::remove(KSycoca::absoluteFilePath());
    KSycoca::self()->ensureCacheValid();
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath()));
}

void TaskToolsTest::cleanupTestCase()
{
    QFile::remove(KSycoca::absoluteFilePath());
}

void TaskToolsTest::shouldFindApp()
{
    // FIXME Test icon.

    const AppData &data = appDataFromUrl(QUrl::fromLocalFile(appLinkPath()));

    QCOMPARE(data.id, m_referenceAppData.id);
    QCOMPARE(data.name, m_referenceAppData.name);
    QCOMPARE(data.genericName, m_referenceAppData.genericName);
    QCOMPARE(data.url, m_referenceAppData.url);
}

void TaskToolsTest::shouldFindDefaultApp()
{
    // FIXME Test other recognized default app types.

    KConfigGroup config(KSharedConfig::openConfig(), "General");
    config.writePathEntry("BrowserApplication", QLatin1String("konqueror"));

    QVERIFY(defaultApplication(QUrl("wrong://url")).isEmpty());
    QCOMPARE(defaultApplication(QUrl("preferred://browser")), QLatin1String("konqueror"));
}

void TaskToolsTest::shouldCompareLauncherUrls()
{
    QUrl a(QLatin1String("file:///usr/share/applications/org.kde.dolphin.desktop"));
    QUrl b(QLatin1String("file:///usr/share/applications/org.kde.konsole.desktop"));
    QUrl c(QLatin1String("file:///usr/share/applications/org.kde.dolphin.desktop?iconData=foo"));
    QUrl d(QLatin1String("file:///usr/share/applications/org.kde.konsole.desktop?iconData=bar"));

    QVERIFY(launcherUrlsMatch(QUrl(a), QUrl(a)));
    QVERIFY(launcherUrlsMatch(QUrl(a), QUrl(a), Strict));
    QVERIFY(launcherUrlsMatch(QUrl(a), QUrl(a), IgnoreQueryItems));

    QVERIFY(!launcherUrlsMatch(QUrl(a), QUrl(b)));
    QVERIFY(!launcherUrlsMatch(QUrl(a), QUrl(b), Strict));
    QVERIFY(!launcherUrlsMatch(QUrl(a), QUrl(b), IgnoreQueryItems));

    QVERIFY(!launcherUrlsMatch(QUrl(b), QUrl(c), Strict));
    QVERIFY(!launcherUrlsMatch(QUrl(c), QUrl(d), Strict));

    QVERIFY(launcherUrlsMatch(QUrl(a), QUrl(c), IgnoreQueryItems));
    QVERIFY(!launcherUrlsMatch(QUrl(c), QUrl(d), IgnoreQueryItems));
}

QString TaskToolsTest::appLinkPath()
{
    return QString(m_tempDir.path() + QLatin1String("/data/applications/org.kde.konversation.desktop"));
}

void TaskToolsTest::fillReferenceAppData()
{
    // FIXME Add icon.

    m_referenceAppData.id = QLatin1String("org.kde.konversation");
    m_referenceAppData.name = QLatin1String("Konversation");
    m_referenceAppData.genericName = QLatin1String("IRC Client");
    m_referenceAppData.url = QUrl("applications:org.kde.konversation.desktop");
}

void TaskToolsTest::createAppLink()
{
    KDesktopFile file(appLinkPath());
    KConfigGroup group = file.desktopGroup();
    group.writeEntry(QLatin1String("Type"), QStringLiteral("Application"));
    group.writeEntry(QLatin1String("Name"), m_referenceAppData.name);
    group.writeEntry(QLatin1String("GenericName"), m_referenceAppData.genericName);
    group.writeEntry(QLatin1String("Icon"), QStringLiteral("konversation"));
    group.writeEntry(QLatin1String("Exec"), QStringLiteral("konversation"));
    file.sync();

    QVERIFY(file.hasApplicationType());

    QVERIFY(QFile::exists(appLinkPath()));
    QVERIFY(KDesktopFile::isDesktopFile(appLinkPath()));
}

void TaskToolsTest::createIcon()
{
    // FIXME KIconLoaderPrivate::initIconThemes: Error: standard icon theme "oxygen" not found!

    QString iconDir = m_tempDir.path() + QLatin1String("/data/icons/");

    QVERIFY(QDir().mkpath(iconDir));

    QIcon::setThemeSearchPaths(QStringList() << m_tempDir.path() + QLatin1String("/data/icons/"));
    QCOMPARE(QIcon::themeSearchPaths(), QStringList() << iconDir);

    iconDir = iconDir + QLatin1String("/") + KIconTheme::defaultThemeName();

    QVERIFY(QDir().mkpath(iconDir));

    const QString &themeFile = iconDir + QLatin1String("/index.theme");

    KConfig config(themeFile);
    KConfigGroup group(config.group(QLatin1String("Icon Theme")));
    group.writeEntry(QLatin1String("Name"), KIconTheme::defaultThemeName());
    group.writeEntry(QLatin1String("Inherits"), QStringLiteral("hicolor"));
    config.sync();

    QVERIFY(QFile::exists(themeFile));

    iconDir = iconDir + QLatin1String("/64x64/apps");

    QVERIFY(QDir().mkpath(iconDir));

    const QString &iconPath = iconDir + QLatin1String("/konversation.png");

    QImage image(64, 64, QImage::Format_Mono);
    image.save(iconPath);

    QVERIFY(QFile::exists(iconPath));
}

QTEST_MAIN(TaskToolsTest)

#include "tasktoolstest.moc"
