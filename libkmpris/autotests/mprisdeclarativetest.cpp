/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#define QT_FORCE_ASSERTS 1

#include <QConcatenateTablesProxyModel>
#include <QProcess>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTest>

#include "mpris2sourcemodel.h"
#include "mprisinterface.h"

using namespace Qt::StringLiterals;

namespace
{
template<typename T>
T evaluate(QObject *scope, const QString &expression)
{
    QQmlExpression expr(qmlContext(scope), scope, expression);
    QVariant result = expr.evaluate();
    if (expr.hasError()) {
        qWarning() << expr.error().toString();
    }
    return result.value<T>();
}

bool initView(QQuickView *view, const QString &urlStr)
{
    view->setSource(QUrl(urlStr));

    QSignalSpy statusSpy(view, &QQuickView::statusChanged);
    if (view->status() == QQuickView::Loading) {
        statusSpy.wait();
    } else if (view->status() != QQuickView::Ready) {
        qCritical() << "Not loading" << view->errors();
        return false;
    }

    if (view->status() != QQuickView::Ready) {
        qCritical() << view->errors();
        return false;
    }

    return true;
}
}

class MprisDeclarativeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cleanupTestCase();

    /**
     * Loads Mpris2Model in QML, and tests a few functions
     */
    void test_Mpris2Model();

    /**
     * Loads MultiplexerModel in QML
     */
    void test_MultiplexerModel();

    /**
     * Disables the multiplexer
     */
    void test_disableMultiplexer();

    /**
     * Sets the preferred player
     */
    void test_preferredPlayer();

private:
    QProcess *m_playerProcess = MprisInterface::startPlayer(this);
};

void MprisDeclarativeTest::cleanupTestCase()
{
    MprisInterface::stopPlayer(m_playerProcess);
}

void MprisDeclarativeTest::test_Mpris2Model()
{
    auto view = std::make_unique<QQuickView>();
    QByteArray errorMessage;
    QVERIFY(initView(view.get(), QFINDTESTDATA(QStringLiteral("data/tst_Mpris2Model.qml"))));

    QQuickItem *rootObject = view->rootObject();
    QSignalSpy countSpy(rootObject, SIGNAL(countChanged()));
    bool ok = false;
    int count = rootObject->property("count").toInt(&ok);
    QVERIFY(ok);

    if (count == 0) {
        QVERIFY(countSpy.wait());
    }
    QCOMPARE(evaluate<unsigned>(rootObject, QStringLiteral("count")), 1);

    auto currentPlayer = evaluate<QObject *>(rootObject, QLatin1String("mpris.currentPlayer"));
    QVERIFY(currentPlayer);
    auto currentIndex = evaluate<unsigned>(rootObject, QLatin1String("mpris.currentIndex"));
    QCOMPARE(currentIndex, 0u);

    // Mpris2Model::playerForLauncherUrl
    auto playerContainerFromPid = evaluate<QObject *>(
        rootObject,
        QLatin1String("mpris.playerForLauncherUrl(\"applications:invalidlauncherurl.desktop\", %1)").arg(QString::number(m_playerProcess->processId())));
    QCOMPARE(currentPlayer, playerContainerFromPid);

    auto playerContainerFromLauncherUrl = evaluate<QObject *>(rootObject, QLatin1String("mpris.playerForLauncherUrl(\"applications:audacious.desktop\", 0)"));
    QCOMPARE(currentPlayer, playerContainerFromLauncherUrl);

    // Make sure it's a real player container, and make sure PlayerContainer is registered
    QCOMPARE(evaluate<QString>(rootObject, QStringLiteral("mpris.currentPlayer.identity")), QStringLiteral("Audacious"));

    // Make sure an unrelated launcher doesn't have any associated player container.
    // This checks the `pid > 0` condition when matching KDEPidRole
    auto launcherShouldNotHavePlayerContainer =
        evaluate<QObject *>(rootObject, QLatin1String("mpris.playerForLauncherUrl(\"applications:dolphin.desktop\", 0)"));
    QVERIFY(!launcherShouldNotHavePlayerContainer);
}

void MprisDeclarativeTest::test_MultiplexerModel()
{
    auto view = std::make_unique<QQuickView>();
    QByteArray errorMessage;
    QVERIFY(initView(view.get(), QFINDTESTDATA(QStringLiteral("data/tst_MultiplexerModel.qml"))));

    QQuickItem *rootObject = view->rootObject();
    QSignalSpy countSpy(rootObject, SIGNAL(countChanged()));
    bool ok = false;
    int count = rootObject->property("count").toInt(&ok);
    QVERIFY(ok);

    if (count == 0) {
        QVERIFY(countSpy.wait());
    }
    QCOMPARE(evaluate<unsigned>(rootObject, QStringLiteral("count")), 1);

    QCOMPARE(evaluate<QString>(rootObject, QStringLiteral("modelData.identity")), QStringLiteral("Choose player automatically"));
}

void MprisDeclarativeTest::test_disableMultiplexer()
{
    QQuickView view;
    QByteArray errorMessage;
    QVERIFY(initView(&view, QFINDTESTDATA(QStringLiteral("data/tst_DisableMultiplexer.qml"))));
    QQuickItem *rootObject = view.rootObject();
    auto model = static_cast<QConcatenateTablesProxyModel *>(rootObject->property("model").value<QObject *>());
    QSignalSpy rowsInsertedSpy(model, &QConcatenateTablesProxyModel::rowsInserted);
    if (model->rowCount() != 1) {
        QVERIFY(rowsInsertedSpy.wait());
    }
    QCOMPARE(model->rowCount(), 1);
    QString first = get<QString>(model->index(0, 0).data(Mpris2SourceModel::IdentityRole));
    QCOMPARE(first, u"Audacious"_s);

    // Enable on the fly
    rowsInsertedSpy.clear();
    model->setProperty("multiplexerEnabled", true);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(get<QString>(model->index(0, 0).data(Mpris2SourceModel::IdentityRole)), u"Choose player automatically"_s);

    // Disable on the fly
    rowsInsertedSpy.clear();
    model->setProperty("multiplexerEnabled", false);
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(get<QString>(model->index(0, 0).data(Mpris2SourceModel::IdentityRole)), u"Audacious"_s);
}

void MprisDeclarativeTest::test_preferredPlayer()
{
    QQuickView view;
    QByteArray errorMessage;
    QVERIFY(initView(&view, QFINDTESTDATA(QStringLiteral("data/tst_PreferredPlayer.qml"))));
    QQuickItem *rootObject = view.rootObject();
    auto model = static_cast<QConcatenateTablesProxyModel *>(rootObject->property("model").value<QObject *>());
    QSignalSpy rowsInsertedSpy(model, &QConcatenateTablesProxyModel::rowsInserted);
    if (model->rowCount() != 2) {
        QVERIFY(rowsInsertedSpy.wait());
        if (model->rowCount() != 2) {
            QVERIFY(rowsInsertedSpy.wait());
        }
    }
    QCOMPARE(model->rowCount(), 2);

    QString first = get<QString>(model->index(0, 0).data(Qt::DisplayRole));
    const QString second = get<QString>(model->index(1, 0).data(Qt::DisplayRole));
    QCOMPARE(first, second);
    QDBusMessage msg = QDBusMessage::createMethodCall(u"org.mpris.MediaPlayer2.appiumtest.instance%1"_s.arg(QString::number(m_playerProcess->processId())),
                                                      u"/org/mpris/MediaPlayer2"_s,
                                                      u"org.mpris.MediaPlayer2.Player"_s,
                                                      u"Play"_s);
    QDBusReply<void> reply = QDBusConnection::sessionBus().call(msg);
    QVERIFY(reply.isValid());

    rowsInsertedSpy.clear();
    QProcess *playerA = MprisInterface::startPlayer(this, u"player_a.json"_s); // desktopentry: appiumtests
    QScopeGuard stopPlayer([playerA] {
        MprisInterface::stopPlayer(playerA);
    });
    if (rowsInsertedSpy.empty()) {
        QVERIFY(rowsInsertedSpy.wait());
    }
    QCOMPARE(model->rowCount(), 3);

    const QString third = get<QString>(model->index(2, 0).data(Qt::DisplayRole));
    QTRY_COMPARE(get<QString>(model->index(0, 0).data(Qt::DisplayRole)), third); // Prefer "appiumtests" even if it is not playing

    // Update the preferred player
    model->setProperty("preferredPlayer", QString());
    QTRY_COMPARE(get<QString>(model->index(0, 0).data(Qt::DisplayRole)), second);
}

QTEST_MAIN(MprisDeclarativeTest)

#include "mprisdeclarativetest.moc"
