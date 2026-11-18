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
        cleanupTestCase();
    }

    void cleanupTestCase()
    {
        QFile::remove(m_desktopPath + u'/' + m_testFileName);
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
        connect(job, &KIO::Job::result, this, [this, job]() {
            QCOMPARE(job->error(), 0);
            QVERIFY(QFile::exists(m_desktopPath + u'/' + m_testFileName));
        });
    }

    void testMostLocalUrl() // relies on testCopyToDesktop being run before
    {
        const QUrl desktopUrl(QString(u"desktop:/" + m_testFileName));
        const QString filePath(m_desktopPath + u'/' + m_testFileName);
        KIO::StatJob *job = KIO::mostLocalUrl(desktopUrl, KIO::HideProgressInfo);
        QVERIFY(job);
        connect(job, &KIO::Job::result, this, [job, filePath]() {
            QCOMPARE(job->error(), 0);
            QCOMPARE(job->mostLocalUrl().toLocalFile(), filePath);
        });
    }

private:
    QString m_desktopPath;
    QString m_testFileName;
};

QTEST_GUILESS_MAIN(TestDesktop)

#include "kio_desktop_test.moc"
