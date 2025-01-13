/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <chrono>

#include <KDirNotify>
#include <KIO/CopyJob>
#include <KIO/FileCopyJob>
#include <KIO/FileUndoManager>
#include <KIO/StatJob>
#include <QDesktopServices>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>
#include <kdirlister.h>
#include <kio/copyjob.h>
#include <kio/job.h>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

static void doUndo() // see FileUndoManagerTest::doUndo()
{
    QEventLoop eventLoop;
    QObject::connect(KIO::FileUndoManager::self(), &KIO::FileUndoManager::undoJobFinished, &eventLoop, &QEventLoop::quit);
    KIO::FileUndoManager::self()->undo();
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents); // wait for undo job to finish
}

class TestDesktop : public QObject
{
    Q_OBJECT

public:
    TestDesktop()
    {
    }

private Q_SLOTS:
    void initTestCase()
    {
        // make KIOs use test mode too
        qputenv("KIOWORKER_ENABLE_TESTMODE", "1");
        QStandardPaths::setTestModeEnabled(true);

        // Warning: even with test mode enabled, this is the real user's Desktop directory
        m_desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        m_testFileName = QLatin1String("kio_desktop_test_file");
        qWarning() << "We will write in the desktop folder" << m_desktopPath;
        // NOTE: we must shut down kded if we started it in the CI! Do not detach!
        m_kded.setProgram(u"kded6"_s);
        m_kded.start();
        const auto started = m_kded.waitForStarted((1000ms).count());
        qWarning() << "Starting kded" << started;
        cleanupTestCase();
    }
    void cleanupTestCase()
    {
        QFile::remove(m_desktopPath + u'/' + m_testFileName);
        QFile::remove(m_desktopPath + u'/' + m_testFileName + ".part"_L1);
        QFile::remove(m_desktopPath + u'/' + m_testFileName + "_link"_L1);
        m_kded.kill();
    }

    void testCopyToDesktop()
    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write("Hello world\n", 12);
        QString fileName = tempFile.fileName();
        tempFile.close();
        KIO::Job *job = KIO::file_copy(QUrl::fromLocalFile(fileName), QUrl(QString(u"desktop:/" + m_testFileName)), -1, KIO::HideProgressInfo);
        job->setUiDelegate(nullptr);
        QVERIFY(job->exec());
        QVERIFY(QFile::exists(m_desktopPath + u'/' + m_testFileName));
    }

    void testMostLocalUrl() // relies on testCopyToDesktop being run before
    {
        const QUrl desktopUrl(QString(u"desktop:/" + m_testFileName));
        const QString filePath(m_desktopPath + u'/' + m_testFileName);
        KIO::StatJob *job = KIO::mostLocalUrl(desktopUrl, KIO::HideProgressInfo);
        QVERIFY(job);
        bool ok = job->exec();
        QVERIFY(ok);
        QCOMPARE(job->mostLocalUrl().toLocalFile(), filePath);
    }

    void testCreateSymlink()
    {
        const QUrl desktopUrl(QString(u"desktop:/" + m_testFileName));
        const QUrl desktopLink(QString(u"desktop:/" + m_testFileName + u"_link"));
        const QString source = m_desktopPath + u'/' + m_testFileName;
        const QString localLink = source + u"_link";

        // Create a symlink using kio_desktop
        KIO::Job *linkJob = KIO::symlink(m_testFileName, desktopLink, KIO::HideProgressInfo);
        QVERIFY(linkJob->exec());
        QVERIFY(QFileInfo(localLink).isSymLink());
        QCOMPARE(QFileInfo(localLink).symLinkTarget(), source);

        // Now try changing the link target, without Overwrite -> error
        linkJob = KIO::symlink(m_testFileName + QLatin1Char('2'), desktopLink, KIO::HideProgressInfo);
        QVERIFY(!linkJob->exec());
        QCOMPARE(linkJob->error(), (int)KIO::ERR_FILE_ALREADY_EXIST);

        // Now try changing the link target, with Overwrite (bug 360487)
        linkJob = KIO::symlink(m_testFileName + QLatin1Char('3'), desktopLink, KIO::Overwrite | KIO::HideProgressInfo);
        QVERIFY(linkJob->exec());
        QVERIFY(QFileInfo(localLink).isSymLink());
        QCOMPARE(QFileInfo(localLink).symLinkTarget(), source + QLatin1Char('3'));
    }

    void testRename_data()
    {
        QTest::addColumn<bool>("withDirListerCache");
        QTest::addColumn<QUrl>("srcUrl");
        QTest::addColumn<QUrl>("destUrl");

        const QString str = u"desktop:/" + m_testFileName;
        const QUrl orig(str);
        const QUrl part(QString(str + u".part"));
        QTest::newRow("orig_to_part") << false << orig << part;
        QTest::newRow("part_to_orig") << false << part << orig;
        // Warnings: all tests without dirlister cache should above this line
        // and all tests with it should be below - the cache stays forever once it exists.
        QTest::newRow("orig_to_part_with_cache") << true << orig << part;
        QTest::newRow("part_to_orig_with_cache") << true << part << orig; // #218719
    }

    void testRename() // relies on testCopyToDesktop being run before
    {
        QFETCH(bool, withDirListerCache);
        QFETCH(QUrl, srcUrl);
        QFETCH(QUrl, destUrl);

        std::unique_ptr<KDirLister> lister;
        std::unique_ptr<QSignalSpy> refreshItemSpy;
        if (withDirListerCache) {
            lister.reset(new KDirLister);
            lister->openUrl(QUrl(QStringLiteral("desktop:/")));
            QSignalSpy spyCompleted(lister.get(), static_cast<void (KDirLister::*)()>(&KDirLister::completed));
            spyCompleted.wait();

            refreshItemSpy.reset(new QSignalSpy(lister.get(), &KDirLister::refreshItems));
        }

        org::kde::KDirNotify kdirnotify(QString(), QString(), QDBusConnection::sessionBus(), this);
        QSignalSpy spyFilesAdded(&kdirnotify, &org::kde::KDirNotify::FilesAdded);
        QSignalSpy spyFileRenamed(&kdirnotify, &org::kde::KDirNotify::FileRenamed);
        QSignalSpy spyFileRenamedWithLocalPath(&kdirnotify, &org::kde::KDirNotify::FileRenamedWithLocalPath);

        const QString srcFilePath(m_desktopPath + srcUrl.path());
        QVERIFY(QFile::exists(srcFilePath));
        const QString destFilePath(m_desktopPath + destUrl.path());
        QVERIFY(!QFile::exists(destFilePath));

        KIO::Job *job = KIO::rename(srcUrl, destUrl, KIO::HideProgressInfo);
        job->setUiDelegate(nullptr);
        QVERIFY(job);
        bool ok = job->exec();
        QVERIFY(ok);
        QVERIFY(!QFile::exists(srcFilePath));
        QVERIFY(QFile::exists(destFilePath));

        // kio_desktop's rename() calls org::kde::KDirNotify::emitFileRenamedWithLocalPath
        // FIXME: fix the double signal emission of fileRenamed
        // and then desktopnotifier notices something changed and emits KDirNotify::FilesAdded
        QTRY_VERIFY(spyFileRenamed.count() >= 1);
        QTRY_VERIFY(spyFileRenamedWithLocalPath.count() >= 1);

        // check that KDirLister now has the correct item (#382341)
        // KDirLister only updates targetUrl during a list job, so targetUrl() is not always the latest.
        if (lister) {
            QVERIFY(!refreshItemSpy->empty() || refreshItemSpy->wait());
            QCOMPARE(lister->findByUrl(destUrl).localPath(), destFilePath);
        }
    }

    void testTrashAndUndo()
    {
        // Given a file on the desktop...
        const QString localPath = m_desktopPath + u'/' + m_testFileName;
        QVERIFY(QFile::exists(localPath));

        // ...moved to the trash
        const QUrl desktopUrl(QString(u"desktop:/" + m_testFileName));
        KIO::Job *job = KIO::trash({desktopUrl}, KIO::HideProgressInfo);
        job->setUiDelegate(nullptr);
        KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Trash, {desktopUrl}, QUrl(QStringLiteral("trash:/")), job);
        QVERIFY2(job->exec(), qPrintable(job->errorString()));
        QVERIFY(!QFile::exists(localPath));

        // When the user calls undo
        doUndo();

        // Then the file should re-appear
        QVERIFY(QFile::exists(localPath));
    }

private:
    QString m_desktopPath;
    QString m_testFileName;
    QProcess m_kded;
};

QTEST_GUILESS_MAIN(TestDesktop)

#include "kio_desktop_test.moc"
