/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#define QT_FORCE_ASSERTS 1

#include <QProcess>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTest>

#include "mprisinterface.h"

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
        QLatin1String("mpris.playerForLauncherUrl(\"application:invalidlauncherurl.desktop\", %1)").arg(QString::number(m_playerProcess->processId())));
    QCOMPARE(currentPlayer, playerContainerFromPid);

    auto playerContainerFromLauncherUrl = evaluate<QObject *>(rootObject, QLatin1String("mpris.playerForLauncherUrl(\"application:audacious.desktop\", 0)"));
    QCOMPARE(currentPlayer, playerContainerFromLauncherUrl);

    // Make sure it's a real player container, and make sure PlayerContainer is registered
    QCOMPARE(evaluate<QString>(rootObject, QStringLiteral("mpris.currentPlayer.identity")), QStringLiteral("Audacious"));
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

QTEST_MAIN(MprisDeclarativeTest)

#include "mprisdeclarativetest.moc"
