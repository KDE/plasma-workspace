/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#define QT_FORCE_ASSERTS 1

#include <QProcess>
#include <QSignalSpy>
#include <QTest>

#include "mpris2filterproxymodel.h"
#include "mpris2model.h"

class FetchInitialPlayerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cleanup();

    /**
     * Make sure if a player is already running before the media controller widget is added,
     * the widget can still provide player information.
     * @see plasma-workspace!9c5726ab353487580297c83ebe4ae67d14cd8830
     */
    void test_playerBeforeModel();

private:
    QProcess *m_playerProcess = nullptr;
};

void FetchInitialPlayerTest::cleanup()
{
    if (m_playerProcess) {
        QSignalSpy finishedSpy(m_playerProcess, &QProcess::finished);
        m_playerProcess->terminate();
        if (m_playerProcess->state() == QProcess::Running) {
            QVERIFY(finishedSpy.wait());
        }
        delete m_playerProcess;
        m_playerProcess = nullptr;
    }
}

void FetchInitialPlayerTest::test_playerBeforeModel()
{
    m_playerProcess = new QProcess(this);
    m_playerProcess->setProgram(QStringLiteral("python3"));
    m_playerProcess->setArguments({QFINDTESTDATA(QStringLiteral("../../appiumtests/utils/mediaplayer.py")), //
                                   QFINDTESTDATA(QStringLiteral("../../appiumtests/resources/player_b.json"))});
    QSignalSpy startedSpy(m_playerProcess, &QProcess::started);
    QSignalSpy readyReadSpy(m_playerProcess, &QProcess::readyReadStandardOutput);
    m_playerProcess->setReadChannel(QProcess::StandardOutput);
    m_playerProcess->start(QIODeviceBase::ReadOnly);
    if (m_playerProcess->state() != QProcess::Running) {
        QVERIFY(startedSpy.wait());
    }

    bool registered = false;
    for (int i = 0; i < 10; ++i) {
        if (m_playerProcess->isReadable() && m_playerProcess->readAllStandardOutput().contains("MPRIS registered")) {
            registered = true;
            break;
        }
        readyReadSpy.wait();
    }
    qDebug() << m_playerProcess->state();
    qDebug() << m_playerProcess->readAllStandardError();
    QVERIFY(registered);

    std::unique_ptr<Mpris2Model> model1 = std::make_unique<Mpris2Model>();
    QSignalSpy rowsInsertedSpy1(model1.get(), &QAbstractItemModel::rowsInserted);
    if (model1->rowCount() == 0) {
        QVERIFY(rowsInsertedSpy1.wait());
    }

    // Because the filter model is shared in the same process, the bug is only testable when a second Mpris2Model is created.
    std::unique_ptr<Mpris2Model> model2 = std::make_unique<Mpris2Model>();
    QVERIFY(model2->rowCount() > 0);
    QVERIFY(model2->currentPlayer());
}

QTEST_GUILESS_MAIN(FetchInitialPlayerTest)

#include "fetchinitialplayertest.moc"
