/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QtTest>

#include "../slidefiltermodel.h"
#include "../slidemodel.h"
#include "commontestdata.h"

#include <array>

class SlideFilterModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testSlideFilterModelSortingOrder_data();
    void testSlideFilterModelSortingOrder();
    void testSlideFilterModelSortingRandomOrder();
    void testSlideFilterModelUncheckedSlides();

private:
    QPointer<SlideModel> m_model = nullptr;
    QPointer<SlideFilterModel> m_filterModel = nullptr;

    QDir m_dataDir;
    QString m_wallpaperPath;
    QString m_standardPath;
    QString m_pathA, m_pathB, m_pathC;
    QSize m_targetSize;
};

void SlideFilterModelTest::initTestCase()
{
    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    QVERIFY(!m_dataDir.isEmpty());

    m_wallpaperPath = m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1);

    m_targetSize = QSize(1920, 1080);

    QStandardPaths::setTestModeEnabled(true);

    m_standardPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/");
    QDir imageDir(m_standardPath);

    QVERIFY(imageDir.mkpath(m_standardPath + QStringLiteral("a/")));
    QVERIFY(imageDir.mkpath(m_standardPath + QStringLiteral("d/")));

    QFile imageFile(m_wallpaperPath);

    m_pathB = imageDir.absoluteFilePath(QStringLiteral("d/b.jpg"));
    QVERIFY(imageFile.copy(m_pathB));

    // Explicitly wait 1s
    QThread::sleep(1);

    m_pathA = imageDir.absoluteFilePath(QStringLiteral("d/a.jpg"));
    QVERIFY(imageFile.copy(m_pathA));

    QThread::sleep(1);

    m_pathC = imageDir.absoluteFilePath(QStringLiteral("a/c.jpg"));
    QVERIFY(imageFile.copy(m_pathC));
}

void SlideFilterModelTest::init()
{
    m_model = new SlideModel(m_targetSize, this);
    m_filterModel = new SlideFilterModel(this);

    // Test loading data
    m_model->setSlidePaths({m_standardPath});
    QSignalSpy doneSpy(m_model, &SlideModel::done);

    QVERIFY(doneSpy.wait(10 * 1000));
    QCOMPARE(doneSpy.size(), 1);
    m_filterModel->setSourceModel(m_model);
}

void SlideFilterModelTest::cleanup()
{
    m_model->deleteLater();
    m_filterModel->deleteLater();
}

void SlideFilterModelTest::cleanupTestCase()
{
    QDir(m_standardPath).removeRecursively();
}

void SlideFilterModelTest::testSlideFilterModelSortingOrder_data()
{
    QTest::addColumn<SortingMode::Mode>("order");
    QTest::addColumn<bool>("folderFirst");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("Alphabetical 0") << SortingMode::Alphabetical << false << QStringList{m_pathA, m_pathB, m_pathC};
    QTest::newRow("Alphabetical 1") << SortingMode::Alphabetical << true << QStringList{m_pathC, m_pathA, m_pathB};

    QTest::newRow("AlphabeticalReversed 0") << SortingMode::AlphabeticalReversed << false << QStringList{m_pathC, m_pathB, m_pathA};
    QTest::newRow("AlphabeticalReversed 1") << SortingMode::AlphabeticalReversed << true << QStringList{m_pathB, m_pathA, m_pathC};

    // Oldest first
    QTest::newRow("Modified 0") << SortingMode::Modified << false << QStringList{m_pathB, m_pathA, m_pathC};
    QTest::newRow("Modified 1") << SortingMode::Modified << true << QStringList{m_pathB, m_pathA, m_pathC};

    // Newest first
    QTest::newRow("ModifiedReversed 0") << SortingMode::ModifiedReversed << false << QStringList{m_pathC, m_pathA, m_pathB};
    QTest::newRow("ModifiedReversed 1") << SortingMode::ModifiedReversed << true << QStringList{m_pathC, m_pathA, m_pathB};
}

void SlideFilterModelTest::testSlideFilterModelSortingOrder()
{
    QFETCH(SortingMode::Mode, order);
    QFETCH(bool, folderFirst);
    QFETCH(QStringList, expected);

    m_filterModel->setSortingMode(order, folderFirst);
    m_filterModel->sort(0);
    QCOMPARE(m_filterModel->rowCount(), 3);

    for (int i = 0; i < expected.size(); i++) {
        QCOMPARE(m_filterModel->index(i, 0).data(ImageRoles::PackageNameRole).toString(), expected.at(i));
    }
}

void SlideFilterModelTest::testSlideFilterModelSortingRandomOrder()
{
    std::array<int, 3> counts{0, 0, 0};
    QCOMPARE(m_filterModel->rowCount(), counts.size());

    // Monte Carlo
    for (int i = 0; i < 1000; i++) {
        m_filterModel->setSortingMode(SortingMode::Random, false);
        m_filterModel->sort(0);

        const QString firstElement = m_filterModel->index(0, 0).data(ImageRoles::PackageNameRole).toString();

        if (firstElement == m_pathA) {
            counts[0]++;
        } else if (firstElement == m_pathB) {
            counts[1]++;
        } else if (firstElement == m_pathC) {
            counts[2]++;
        }
    }

    for (int c : std::as_const(counts)) {
        QVERIFY(std::clamp(c, 200, 400) == c);
    }
}

void SlideFilterModelTest::testSlideFilterModelUncheckedSlides()
{
    m_model->setUncheckedSlides({m_pathA});

    // usedInConfig false
    m_filterModel->setProperty("usedInConfig", false);
    QCOMPARE(m_filterModel->rowCount(), 2);

    m_filterModel->setProperty("usedInConfig", true);
    QCOMPARE(m_filterModel->rowCount(), 3);
}

QTEST_MAIN(SlideFilterModelTest)

#include "test_slidefiltermodel.moc"
