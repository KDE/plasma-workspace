/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickView>
#include <QtTest>

#include "plasmawindowedcorona.h"
#include "plasmawindowedview.h"

namespace
{
template<typename T>
static T evaluate(QObject *scope, const char *expression)
{
    QQmlExpression expr(qmlContext(scope), scope, QString::fromLatin1(expression));
    QVariant result = expr.evaluate();
    if (expr.hasError()) {
        qWarning() << expr.error().toString();
    }
    return result.value<T>();
}

template<>
void evaluate<void>(QObject *scope, const char *expression)
{
    QQmlExpression expr(qmlContext(scope), scope, QString::fromLatin1(expression));
    expr.evaluate();
    if (expr.hasError()) {
        qWarning() << expr.error().toString();
    }
}
}

class ClipboardTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testCopyText();

private:
    QQuickItem *fullRepresentationItem() const;
    QQuickItem *rootItem() const;

    QPointer<PlasmaWindowedCorona> m_corona;
    QPointer<QQuickItem> m_currentItemInStackView;
};

void ClipboardTest::initTestCase()
{
    QGuiApplication::setQuitOnLastWindowClosed(false);
    QStandardPaths::setTestModeEnabled(true);

    // Set klipperrc
    auto config = KSharedConfig::openConfig(QStringLiteral("klipperrc"), KConfig::NoGlobals);
    KConfigGroup defaultGroup = config->group("General");
    defaultGroup.writeEntry("IgnoreImages", false, KConfigBase::Notify); // To test copy image
    defaultGroup.writeEntry("MaxClipItems", 30, KConfigBase::Notify); // To test maximum number of records
    defaultGroup.writeEntry("Version", "5.26.80", KConfigBase::Notify);
    config->sync();
}

void ClipboardTest::init()
{
    Q_ASSERT(!m_corona);
    m_corona = new PlasmaWindowedCorona(QStringLiteral("org.kde.plasma.desktop"), QApplication::instance());
    m_corona->loadApplet(QStringLiteral("org.kde.plasma.clipboard"), QVariantList());
    // Loading
    auto view = m_corona->view();
    QQuickItem *const appletInterface = view->appletInterface();
    Q_ASSERT(appletInterface);
    int waitCount = 0;
    while (!appletInterface->property("fullRepresentationItem").value<QQuickItem *>() && waitCount++ < 20) {
        QTest::qWait(250);
    }

    // Get the current item in StackView
    m_currentItemInStackView = evaluate<QQuickItem *>(fullRepresentationItem(), "stack.currentItem");
    QVERIFY(m_currentItemInStackView);
    QVERIFY(evaluate<bool>(m_currentItemInStackView, "this instanceof PlasmaComponents3.ScrollView")); // ClipboardPage.qml and Menu.qml
    QVERIFY(evaluate<bool>(m_currentItemInStackView, "contentItem instanceof ListView")); // Menu.qml
}

void ClipboardTest::cleanup()
{
    Q_ASSERT(m_corona);
    m_corona->view()->close();
    delete m_corona;

    // Delete history
    QDir klipperHistoryFolder(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + QStringLiteral("klipper"));
    QFile historyFile(klipperHistoryFolder.absoluteFilePath(QStringLiteral("history2.lst")));
    historyFile.remove();

    // Delete config
    QDir configDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
    QFile configFile1(configDir.absoluteFilePath(QStringLiteral("plasmawindowed-appletsrc")));
    configFile1.remove();
    QFile configFile2(configDir.absoluteFilePath(QStringLiteral("test_applet_clipboardrc")));
    configFile2.remove();
}

void ClipboardTest::testCopyText()
{
    QQuickItem *const listViewItem = evaluate<QQuickItem *>(m_currentItemInStackView, "contentItem");
    QVERIFY(listViewItem);
    int count = listViewItem->property("count").toInt();
    qDebug() << "Old count" << count;

    auto copyTextAndCheckCount = [listViewItem, &count](const QString &clipboardText, const QString &expectedText, bool increaseCount) {
        QSignalSpy addSpy(listViewItem, SIGNAL(countChanged()));
        QClipboard *const clipboard = QGuiApplication::clipboard();
        // Must use different strings to create a new record
        clipboard->setText(clipboardText);
        addSpy.wait(500);
        qDebug() << "New count" << listViewItem->property("count").toInt();
        count += increaseCount ? 1 : 0;
        QCOMPARE(count, listViewItem->property("count").toInt());

        // Check string match
        QQuickItem *const firstRecordItem = evaluate<QQuickItem *>(listViewItem, "itemAtIndex(0)");
        QVERIFY(firstRecordItem);
        QCOMPARE(clipboardText, evaluate<QString>(firstRecordItem, "DisplayRole"));
        QQuickItem *itemLoader = firstRecordItem->findChild<QQuickItem *>("itemDelegateLoader");
        QVERIFY(itemLoader);
        QVERIFY(evaluate<bool>(itemLoader, "item instanceof TextItemDelegate"));
        QCOMPARE(evaluate<QString>(itemLoader, "item.text"), expectedText);
    };

    // Case 1: Test copy text to add one record in the widget
    const QString datetimeString = QString::number(QDateTime::currentDateTimeUtc().currentMSecsSinceEpoch());
    const QString clipboardText = QStringLiteral("__clipboard_test__%1").arg(datetimeString);
    copyTextAndCheckCount(clipboardText, clipboardText, true);

    // Case 2: Copy the same text again
    copyTextAndCheckCount(clipboardText, clipboardText, false);

    // Case 3: Copy long text
    QString longText;
    longText.reserve(100 + datetimeString.size());
    for (int i = 0; i < 100; ++i) {
        longText.append(QChar('a'));
    }
    longText.append(datetimeString);
    copyTextAndCheckCount(longText, longText.left(100), true);

    // Case 4: first escape any HTML characters to prevent privacy issues
    const QString htmlText = QStringLiteral("&text<br>") + datetimeString;
    copyTextAndCheckCount(htmlText, QStringLiteral("&amp;text&lt;br&gt;") + datetimeString, true);

    // Case 5: color code leading or trailing whitespace, and finally turn line breaks into HTML br tags
    const QString spaceText = QStringLiteral(" Space \t\nNew Line") + datetimeString + QLatin1Char(' ');
    const QString highlightColorString = evaluate<QString>(fullRepresentationItem(), "\"'\" + PlasmaCore.Theme.highlightColor + \"'\"");
    const QString sanitizedSpaceText = QStringLiteral("<font color=%1>␣</font>Space<font color=%1>␣⇥</font><br>New Line").arg(highlightColorString)
        + datetimeString + QStringLiteral("<font color=%1>␣</font>").arg(highlightColorString);
    copyTextAndCheckCount(spaceText, sanitizedSpaceText, true);

    // Case 5: When rendering multiline strings, the final character of each line(except the last) is not deleted
    const QString multilineStrings = QStringLiteral("Hello\nthere\nWorld");
    copyTextAndCheckCount(multilineStrings, QStringLiteral("Hello<br>there<br>World"), true);

    // Case 6: Don't remove the last character of escaped HTML codes
    const QString lastCharacterIsEscaped = QStringLiteral("some&\ntext%1").arg(datetimeString);
    copyTextAndCheckCount(lastCharacterIsEscaped, QStringLiteral("some&amp;<br>text%1").arg(datetimeString), true);

    // Case 7: Some UTF-8 characters, from https://en.wikipedia.org/wiki/Wikipedia:Language_recognition_chart
    const QString utf8String = QStringLiteral("AàêÆÄÇĂÂĈÇÁꞗāéñيঅअਅઅཀБЪΔת漢あ위ㄅកԱაกⴰ");
    copyTextAndCheckCount(utf8String, utf8String, true);
}

QQuickItem *ClipboardTest::fullRepresentationItem() const
{
    QQuickItem *const appletInterface = m_corona->view()->appletInterface();
    Q_ASSERT(appletInterface);
    QQuickItem *const fullRepresentationItem = appletInterface->property("fullRepresentationItem").value<QQuickItem *>();
    Q_ASSERT(fullRepresentationItem);
    return fullRepresentationItem;
}

QQuickItem *ClipboardTest::rootItem() const
{
    QQuickItem *const appletInterface = m_corona->view()->appletInterface();
    Q_ASSERT(appletInterface);
    QQuickItem *const fullRepresentationItem = appletInterface->property("rootItem").value<QQuickItem *>();
    Q_ASSERT(fullRepresentationItem);
    return fullRepresentationItem;
}

QTEST_MAIN(ClipboardTest)

#include "tst_clipboard.moc"
