/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#define QT_FORCE_ASSERTS 1

#include <QProcess>
#include <QSignalSpy>
#include <QTest>

#include "mpris2model.h"
#include "mprisinterface.h"

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
        MprisInterface::stopPlayer(m_playerProcess);
        m_playerProcess = nullptr;
    }
}

void FetchInitialPlayerTest::test_playerBeforeModel()
{
    m_playerProcess = MprisInterface::startPlayer(this);

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
