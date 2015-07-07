/* This file is part of the KDE project
   Copyright (C) 2009 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <kdirlister.h>
#include <ktemporaryfile.h>
#include <kdebug.h>
#include <QDesktopServices>
#include <QObject>
#include <qtest_kde.h>
#include <kio/job.h>
#include <kio/copyjob.h>

class TestDesktop : public QObject
{
    Q_OBJECT

public:
    TestDesktop() {}

private Q_SLOTS:
    void initTestCase()
    {
        setenv( "KDE_FORK_SLAVES", "yes", true );

        // copied from kio_desktop.cpp:
        m_desktopPath = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
        if (m_desktopPath.isEmpty())
            m_desktopPath = QDir::homePath() + "/Desktop";
        // Warning, this defaults to $HOME/Desktop, the _real_ desktop dir.
        m_testFileName = "kio_desktop_test_file";
    }
    void cleanupTestCase()
    {
        QFile::remove(m_desktopPath + '/' + m_testFileName);
        QFile::remove(m_desktopPath + '/' + m_testFileName + ".part");
    }

    void testCopyToDesktop()
    {
        KTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write( "Hello world\n", 12 );
        QString fileName = tempFile.fileName();
        tempFile.close();
        KIO::Job* job = KIO::file_copy(QUrl::fromLocalFile(fileName), QUrl("desktop:/" + m_testFileName), -1, KIO::HideProgressInfo);
        job->setUiDelegate(0);
        QVERIFY(job->exec());
        QVERIFY(QFile::exists(m_desktopPath + '/' + m_testFileName));
    }

    void testMostLocalUrl() // relies on testCopyToDesktop being run before
    {
        const QUrl desktopUrl("desktop:/" + m_testFileName);
        const QString filePath(m_desktopPath + '/' + m_testFileName);
        KIO::StatJob* job = KIO::mostLocalUrl(desktopUrl, KIO::HideProgressInfo);
        QVERIFY(job);
        bool ok = job->exec();
        QVERIFY(ok);
        QCOMPARE(job->mostLocalUrl().toLocalFile(), filePath);
    }

    void testRename_data()
    {
        QTest::addColumn<bool>("withDirListerCache");
        QTest::addColumn<QString>("srcFile");
        QTest::addColumn<QString>("destFile");

        const QString orig = "desktop:/" + m_testFileName;
        const QString part = orig + ".part";
        QTest::newRow("from orig to .part") << false << orig << part;
        QTest::newRow("from .part to orig") << false << part << orig;
        // Warnings: all tests without dirlister cache should above this line
        // and all tests with it should be below - the cache stays forever once it exists.
        QTest::newRow("from orig to .part, with cache") << true << orig << part;
        QTest::newRow("from .part to orig, with cache (#218719)") << true << part << orig;
    }

    void testRename() // relies on testCopyToDesktop being run before
    {
        QFETCH(bool, withDirListerCache);
        QFETCH(QString, srcFile);
        QFETCH(QString, destFile);

        if (withDirListerCache) {
            KDirLister lister;
            lister.openUrl(QUrl("desktop:/"));
            QEventLoop eventLoop;
            connect(&lister, static_cast<void (KDirLister::*)()>(&KDirLister::completed), &eventLoop, &QEventLoop::quit);
            eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
        }

        const QUrl srcUrl = QUrl::fromLocalFile(srcFile);
        const QUrl destUrl = QUrl::fromLocalFile(destFile);

        const QString srcFilePath(m_desktopPath + srcUrl.path());
        QVERIFY(QFile::exists(srcFilePath));
        const QString destFilePath(m_desktopPath + destUrl.path());
        QVERIFY(!QFile::exists(destFilePath));

        KIO::CopyJob* job = KIO::move(srcUrl, destUrl, KIO::HideProgressInfo);
        job->setUiDelegate(0);
        QVERIFY(job);
        bool ok = job->exec();
        QVERIFY(ok);
        QVERIFY(!QFile::exists(srcFilePath));
        QVERIFY(QFile::exists(destFilePath));
    }

private:
    QString m_desktopPath;
    QString m_testFileName;
};

QTEST_KDEMAIN(TestDesktop, NoGUI)

#include "kio_desktop_test.moc"
