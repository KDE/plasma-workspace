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
#include <QQuickItemGrabResult>
#include <QtTest>

#include <KLocalizedString>

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
    void testCopyImage();
    void testCopyFileUrl_data();
    void testCopyFileUrl();

    void testEditRecord();
    void testEditCancel();

    void testGenerateBarcode();

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

void ClipboardTest::testCopyImage()
{
    QQuickItem *const listViewItem = evaluate<QQuickItem *>(m_currentItemInStackView, "contentItem");
    QVERIFY(listViewItem);
    QSignalSpy addSpy(listViewItem, SIGNAL(countChanged()));

    QImage testImage(100, 100, QImage::Format_RGB32);
    testImage.fill(Qt::red);
    QClipboard *const clipboard = QGuiApplication::clipboard();
    clipboard->setText("test");
    addSpy.wait(500);
    clipboard->setImage(testImage);
    addSpy.wait(5000);

    // A new image is added, now verify the first record
    QQuickItem *const firstRecordItem = evaluate<QQuickItem *>(listViewItem, "itemAtIndex(0)");
    QVERIFY(firstRecordItem);
    QCOMPARE(QStringLiteral("▨ ") + i18nd("klipper", "%1x%2 %3bpp", testImage.width(), testImage.height(), testImage.depth()),
             evaluate<QString>(firstRecordItem, "DisplayRole"));
    QQuickItem *itemLoader = firstRecordItem->findChild<QQuickItem *>("itemDelegateLoader");
    QVERIFY(itemLoader);

    // Test correct delegate is used
    QVERIFY(evaluate<bool>(itemLoader, "item instanceof ImageItemDelegate"));
    QQuickItem *itemDelegate = evaluate<QQuickItem *>(itemLoader, "item");

    // Grab one pixel to match color
    QEventLoop loop;
    QImage grabImage;
    auto grabResult = itemDelegate->grabToImage();
    QVERIFY(grabResult);
    loop.connect(grabResult.data(), &QQuickItemGrabResult::ready, this, [&loop, &grabResult, &grabImage] {
        grabImage = grabResult->image();
        grabResult.clear();
        loop.quit();
    });
    loop.exec();
    // Grabbed, now compare two colors
    QCOMPARE(grabImage.pixelColor(0, 0), testImage.pixelColor(0, 0));
}

void ClipboardTest::testCopyFileUrl_data()
{
    QTest::addColumn<QList<QUrl>>("urls");
    QTest::addColumn<QStringList>("labels");
    QTest::newRow("single url") << QList<QUrl>{QUrl::fromLocalFile(QFINDTESTDATA("./CMakeLists.txt"))} << QStringList{QStringLiteral("CMakeLists.txt")};
    QTest::newRow("multiple urls") << QList<QUrl>{QUrl::fromLocalFile(QFINDTESTDATA("./CMakeLists.txt")),
                                                  QUrl::fromLocalFile(QFINDTESTDATA("./tst_clipboard.cpp"))}
                                   << QStringList{QStringLiteral("CMakeLists.txt"), QStringLiteral("tst_clipboard.cpp")};
}

void ClipboardTest::testCopyFileUrl()
{
    QFETCH(QList<QUrl>, urls);
    QFETCH(QStringList, labels);

    QQuickItem *const listViewItem = evaluate<QQuickItem *>(m_currentItemInStackView, "contentItem");
    QVERIFY(listViewItem);
    QSignalSpy addSpy(listViewItem, SIGNAL(countChanged()));
    QClipboard *const clipboard = QGuiApplication::clipboard();
    clipboard->setText("test");
    addSpy.wait(500);
    auto mimeData = new QMimeData;
    mimeData->setUrls(urls);
    clipboard->setMimeData(mimeData);
    addSpy.wait();

    QQuickItem *const firstRecordItem = evaluate<QQuickItem *>(listViewItem, "itemAtIndex(0)");
    QVERIFY(firstRecordItem);
    // From klipper/historyurlitem.cpp
    QString ret;
    {
        bool first = true;
        for (const QUrl &url : urls) {
            if (!first) {
                ret.append(QLatin1Char(' '));
            }
            first = false;
            ret.append(url.toString(QUrl::FullyEncoded));
        }
    }
    QCOMPARE(ret, evaluate<QString>(firstRecordItem, "DisplayRole"));
    QQuickItem *itemLoader = firstRecordItem->findChild<QQuickItem *>("itemDelegateLoader");
    QVERIFY(itemLoader);

    // Test correct delegate is used
    QVERIFY(evaluate<bool>(itemLoader, "item instanceof UrlItemDelegate"));
    QQuickItem *itemDelegate = evaluate<QQuickItem *>(itemLoader, "item");
    QCOMPARE(urls.size(), evaluate<int>(itemDelegate, "previewList.count"));

    // Sleep 1s to make sure the preview is loaded
    QTest::qWait(1000);
    // Test url preview
    for (int i = 0; i < urls.size(); ++i) {
        QQuickItem *urlPreviewDelegate =
            evaluate<QQuickItem *>(itemDelegate, QStringLiteral("previewList.itemAtIndex(%1)").arg(QString::number(i)).toLatin1().constData());
        QVERIFY(urlPreviewDelegate);

        // Test file preview
        QEventLoop loop;
        QImage grabImage;
        auto grabResult = urlPreviewDelegate->grabToImage();
        QVERIFY(grabResult);
        loop.connect(grabResult.data(), &QQuickItemGrabResult::ready, this, [&loop, &grabResult, &grabImage] {
            grabImage = grabResult->image();
            grabResult.clear();
            loop.quit();
        });
        loop.exec();
        // If a preview fails to load, the pixel in the center position will be transparent
        QVERIFY(grabImage.pixelColor(grabImage.width() / 2, grabImage.height() / 2) != Qt::transparent);

        // Test preview label / decodeURIComponent
        QCOMPARE(labels.at(i), evaluate<QString>(urlPreviewDelegate, "label.text"));
    }
}

void ClipboardTest::testEditRecord()
{
    QQuickItem *const listViewItem = evaluate<QQuickItem *>(m_currentItemInStackView, "contentItem");
    QVERIFY(listViewItem);

    QSignalSpy addSpy(listViewItem, SIGNAL(countChanged()));
    QClipboard *const clipboard = QGuiApplication::clipboard();
    // Must use different strings to create a new record
    clipboard->setText(QStringLiteral("Hello World%1").arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
    addSpy.wait(500);
    QCOMPARE(listViewItem->property("count").toInt(), 1);

    QQuickItem *firstRecordItem = evaluate<QQuickItem *>(listViewItem, "itemAtIndex(0)");
    QVERIFY(firstRecordItem);
    const QString firstRecordDisplayRole = evaluate<QString>(firstRecordItem, "DisplayRole");
    QQuickItem *const toolButtonsLoader = firstRecordItem->findChild<QQuickItem *>("toolButtonsLoader");
    QVERIFY(toolButtonsLoader);
    QVERIFY(evaluate<bool>(toolButtonsLoader, "!active")); // Is not current item, so loader is not ready

    // Activate tool buttons
    evaluate<void>(listViewItem, "currentIndex = 0;");
    QCoreApplication::processEvents();
    QQuickItem *const toolButtonsLoaderItem = evaluate<QQuickItem *>(toolButtonsLoader, "item");
    QVERIFY(toolButtonsLoaderItem);

    // Click the button
    QQuickItem *const editButton = toolButtonsLoaderItem->findChild<QQuickItem *>("editToolButton");
    QVERIFY(editButton);
    // Click edit button
    evaluate<void>(editButton, "clicked(null);");
    QCoreApplication::processEvents();
    QTest::qWait(1000);

    m_currentItemInStackView = evaluate<QQuickItem *>(fullRepresentationItem(), "stack.currentItem");
    QVERIFY(m_currentItemInStackView);
    QVERIFY(evaluate<bool>(m_currentItemInStackView, "this instanceof EditPage"));
    QQuickItem *const textArea = m_currentItemInStackView->findChild<QQuickItem *>("textArea");
    QCOMPARE(evaluate<QString>(textArea, "text"), firstRecordDisplayRole);

    // Change text
    evaluate<void>(textArea, "text = 'Goodbye';");
    QCoreApplication::processEvents();

    // Save
    QQuickItem *const saveButton = m_currentItemInStackView->findChild<QQuickItem *>("saveButton");
    QVERIFY(saveButton);
    evaluate<void>(saveButton, "clicked();");
    QCoreApplication::processEvents();

    // Back to list
    m_currentItemInStackView = evaluate<QQuickItem *>(fullRepresentationItem(), "stack.currentItem");
    QVERIFY(m_currentItemInStackView);
    QVERIFY(evaluate<bool>(m_currentItemInStackView, "this instanceof PlasmaComponents3.ScrollView"));

    firstRecordItem = evaluate<QQuickItem *>(listViewItem, "itemAtIndex(0)");
    QVERIFY(firstRecordItem);
    QCOMPARE(evaluate<QString>(firstRecordItem, "DisplayRole"), QStringLiteral("Goodbye"));
}

void ClipboardTest::testEditCancel()
{
    QQuickItem *const listViewItem = evaluate<QQuickItem *>(m_currentItemInStackView, "contentItem");
    QVERIFY(listViewItem);

    QSignalSpy addSpy(listViewItem, SIGNAL(countChanged()));
    QClipboard *const clipboard = QGuiApplication::clipboard();
    // Must use different strings to create a new record
    clipboard->setText(QStringLiteral("Hello World%1").arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
    addSpy.wait(500);
    QCOMPARE(listViewItem->property("count").toInt(), 1);

    QQuickItem *firstRecordItem = evaluate<QQuickItem *>(listViewItem, "itemAtIndex(0)");
    QVERIFY(firstRecordItem);
    const QString firstRecordDisplayRole = evaluate<QString>(firstRecordItem, "DisplayRole");
    QQuickItem *const toolButtonsLoader = firstRecordItem->findChild<QQuickItem *>("toolButtonsLoader");
    QVERIFY(toolButtonsLoader);
    QVERIFY(evaluate<bool>(toolButtonsLoader, "!active")); // Is not current item, so loader is not ready

    // Activate tool buttons
    evaluate<void>(listViewItem, "currentIndex = 0;");
    QCoreApplication::processEvents();
    QQuickItem *const toolButtonsLoaderItem = evaluate<QQuickItem *>(toolButtonsLoader, "item");
    QVERIFY(toolButtonsLoaderItem);

    // Click the button
    QQuickItem *const editButton = toolButtonsLoaderItem->findChild<QQuickItem *>("editToolButton");
    QVERIFY(editButton);
    // Click edit button
    evaluate<void>(editButton, "clicked(null);");
    QCoreApplication::processEvents();
    QTest::qWait(1000);

    m_currentItemInStackView = evaluate<QQuickItem *>(fullRepresentationItem(), "stack.currentItem");
    QVERIFY(m_currentItemInStackView);
    QVERIFY(evaluate<bool>(m_currentItemInStackView, "this instanceof EditPage"));
    QQuickItem *const textArea = m_currentItemInStackView->findChild<QQuickItem *>("textArea");
    QCOMPARE(evaluate<QString>(textArea, "text"), firstRecordDisplayRole);

    // Change text
    evaluate<void>(textArea, "text = 'Goodbye';");
    QCoreApplication::processEvents();

    // Cancel
    QQuickItem *const cancelButton = m_currentItemInStackView->findChild<QQuickItem *>("cancelButton");
    QVERIFY(cancelButton);
    evaluate<void>(cancelButton, "clicked();");
    QCoreApplication::processEvents();

    // Back to list
    m_currentItemInStackView = evaluate<QQuickItem *>(fullRepresentationItem(), "stack.currentItem");
    QVERIFY(m_currentItemInStackView);
    QVERIFY(evaluate<bool>(m_currentItemInStackView, "this instanceof PlasmaComponents3.ScrollView"));

    firstRecordItem = evaluate<QQuickItem *>(listViewItem, "itemAtIndex(0)");
    QVERIFY(firstRecordItem);
    QCOMPARE(evaluate<QString>(firstRecordItem, "DisplayRole"), firstRecordDisplayRole);
}

void ClipboardTest::testGenerateBarcode()
{
    QQuickItem *const listViewItem = evaluate<QQuickItem *>(m_currentItemInStackView, "contentItem");
    QVERIFY(listViewItem);

    QSignalSpy addSpy(listViewItem, SIGNAL(countChanged()));
    QClipboard *const clipboard = QGuiApplication::clipboard();
    // Must use different strings to create a new record
    clipboard->setText(QStringLiteral("Hello World%1").arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
    addSpy.wait(500);
    QCOMPARE(listViewItem->property("count").toInt(), 1);

    QQuickItem *firstRecordItem = evaluate<QQuickItem *>(listViewItem, "itemAtIndex(0)");
    QVERIFY(firstRecordItem);
    QQuickItem *const toolButtonsLoader = firstRecordItem->findChild<QQuickItem *>("toolButtonsLoader");
    QVERIFY(toolButtonsLoader);
    QVERIFY(evaluate<bool>(toolButtonsLoader, "!active")); // Is not current item, so loader is not ready

    // Activate tool buttons
    evaluate<void>(listViewItem, "currentIndex = 0;");
    QCoreApplication::processEvents();
    QQuickItem *const toolButtonsLoaderItem = evaluate<QQuickItem *>(toolButtonsLoader, "item");
    QVERIFY(toolButtonsLoaderItem);

    // Click the button
    QQuickItem *const barcodeToolButton = toolButtonsLoaderItem->findChild<QQuickItem *>("barcodeToolButton");
    QVERIFY(barcodeToolButton);
    // Click edit button
    evaluate<void>(barcodeToolButton, "clicked(null);");
    QCoreApplication::processEvents();
    QTest::qWait(1000); // Animation

    m_currentItemInStackView = evaluate<QQuickItem *>(fullRepresentationItem(), "stack.currentItem");
    QVERIFY(m_currentItemInStackView);
    QVERIFY(evaluate<bool>(m_currentItemInStackView, "this instanceof BarcodePage"));
    QQuickItem *const barcodeItem = m_currentItemInStackView->findChild<QQuickItem *>("barcodeItem");
    QVERIFY(barcodeItem);

    // Grab the image
    QEventLoop loop;
    QImage grabImage;
    auto grabResult = barcodeItem->grabToImage();
    Q_ASSERT(grabResult);
    loop.connect(grabResult.data(), &QQuickItemGrabResult::ready, this, [&loop, &grabResult, &grabImage] {
        grabImage = grabResult->image();
        grabResult.clear();
        loop.quit();
    });
    loop.exec();
    // If a preview fails to load, the pixel in the center position will be transparent
    const QColor centerColor = grabImage.pixelColor(grabImage.width() / 2, grabImage.height() / 2);
    QVERIFY(centerColor == Qt::white || centerColor == Qt::black); // Barcode image
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
