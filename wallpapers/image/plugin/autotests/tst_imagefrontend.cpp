/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QImage>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickView>
#include <QtTest>

#include "commontestdata.h"

namespace
{
class MockWallpaperInterface : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool loading MEMBER m_loading NOTIFY isLoadingChanged)

public:
    explicit MockWallpaperInterface(QObject *parent = nullptr)
        : QObject(parent)
    {
        connect(this, &MockWallpaperInterface::repaintNeeded, this, [this] {
            m_repainted = true;
        });
    }

    bool m_repainted = false; // To be set to true by QQC2.StackView.onActivated
    bool m_loading = true; // To be set to false by replaceWhenLoaded

Q_SIGNALS:
    void isLoadingChanged();
    void repaintNeeded(const QColor &accentColor = Qt::transparent);
};

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

bool initView(QQuickView *view, const QUrl &url, QByteArray *errorMessage)
{
    view->setResizeMode(QQuickView::SizeViewToRootObject);
    view->setSource(url);

    while (view->status() == QQuickView::Loading) {
        QTest::qWait(10);
    }
    if (view->status() != QQuickView::Ready) {
        const auto errors = view->errors();
        for (const QQmlError &e : errors)
            errorMessage->append(e.toString().toLocal8Bit() + '\n');
        return false;
    }

    return true;
}

}

class ImageFrontendTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testLoadWallpaper_data();
    void testLoadWallpaper();

private:
    QPointer<QQuickView> m_view;

    QDir m_dataDir;
};

void ImageFrontendTest::initTestCase()
{
    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));
    QVERIFY(!m_dataDir.isEmpty());

    QStandardPaths::setTestModeEnabled(true);
}

void ImageFrontendTest::init()
{
    Q_ASSERT(!m_view);
    m_view = new QQuickView();
    m_view->engine()->setBaseUrl(QUrl::fromLocalFile(QFINDTESTDATA("../../imagepackage/contents/ui/")));
}

void ImageFrontendTest::cleanup()
{
    Q_ASSERT(m_view);
    delete m_view;
}

void ImageFrontendTest::testLoadWallpaper_data()
{
    QTest::addColumn<int>("fillMode");
    QTest::addColumn<QString>("configColor");
    QTest::addColumn<bool>("blur");
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("modelImage");
    QTest::addColumn<QSize>("sourceSize");
    QTest::addColumn<QColor>("expectedColorAtTopLeft");

    const QString defaultImage = m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1);
    const QImage image(defaultImage);
    const QSize imageSize = image.size();

    // Use different fill modes and colors
    // Test the background color is covered by the image
    Q_ASSERT(image.pixelColor(0, 0) != Qt::blue); // Make sure background color and image color are different
    QTest::newRow("Default") << 2 << QStringLiteral("blue") << false << defaultImage << QUrl::fromLocalFile(defaultImage).toString()
                             << QSize(imageSize.width() + 100, imageSize.height()) << image.pixelColor(0, 0);
    // Test blurred background
    QTest::newRow("Blur enabled") << 1 /* PreserveAspectFit */ << QStringLiteral("blue") << true << defaultImage << QUrl::fromLocalFile(defaultImage).toString()
                                  << QSize(imageSize.width() + 100, imageSize.height()) << image.pixelColor(0, 0);
    // Test background color
    QTest::newRow("Background color") << 6 /* Pad */ << QStringLiteral("blue") << false << defaultImage << QUrl::fromLocalFile(defaultImage).toString()
                                      << imageSize * 2 << QColor(Qt::blue);
}

void ImageFrontendTest::testLoadWallpaper()
{
    QFETCH(int, fillMode);
    QFETCH(QString, configColor);
    QFETCH(bool, blur);
    QFETCH(QString, source);
    QFETCH(QString, modelImage);
    QFETCH(QSize, sourceSize);
    QFETCH(QColor, expectedColorAtTopLeft);

    // Set required properties and window size
    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("fillMode"), fillMode);
    initialProperties.insert(QStringLiteral("configColor"), configColor);
    initialProperties.insert(QStringLiteral("blur"), blur);
    initialProperties.insert(QStringLiteral("source"), source);
    initialProperties.insert(QStringLiteral("sourceSize"), sourceSize);
    initialProperties.insert(QStringLiteral("width"), sourceSize.width());
    initialProperties.insert(QStringLiteral("height"), sourceSize.height());
    auto wpGraphicObject = new MockWallpaperInterface(m_view);
    initialProperties.insert(QStringLiteral("wallpaperInterface"), QVariant::fromValue(wpGraphicObject));
    m_view->setInitialProperties(initialProperties);

    // When repaintNeeded is emitted, the transition animation has finished.
    QSignalSpy repaintSpy(wpGraphicObject, &MockWallpaperInterface::repaintNeeded);

    QByteArray errorMessage;
    QVERIFY2(initView(m_view.data(), QUrl::fromLocalFile(QFINDTESTDATA("../../imagepackage/contents/ui/ImageStackView.qml")), &errorMessage),
             errorMessage.constData());

    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view));
    QQuickItem *rootObject = m_view->rootObject();
    QVERIFY(rootObject);

    // Wait loaded
    QVERIFY(wpGraphicObject->m_repainted || repaintSpy.wait());
    auto currentItem = evaluate<QQuickItem *>(rootObject, "currentItem");
    QVERIFY(currentItem);

    // Check required properties are set correctly
    QCOMPARE(evaluate<int>(rootObject, "fillMode"), fillMode);
    QCOMPARE(evaluate<int>(currentItem, "fillMode"), fillMode);
    QCOMPARE(evaluate<QString>(rootObject, "configColor"), configColor);
    QCOMPARE(evaluate<QColor>(currentItem, "color"), QColor(configColor));
    QCOMPARE(evaluate<bool>(rootObject, "blur"), blur);
    QCOMPARE(evaluate<bool>(currentItem, "blur"), blur);
    QCOMPARE(evaluate<QString>(rootObject, "source"), source);
    QCOMPARE(evaluate<QSize>(rootObject, "sourceSize"), sourceSize);
    QCOMPARE(evaluate<QSize>(currentItem, "sourceSize"), sourceSize);

    // Check modelImage path
    QCOMPARE(evaluate<QString>(rootObject, "modelImage"), modelImage);
    QCOMPARE(evaluate<QUrl>(currentItem, "source").toString(), modelImage);

    // Check color
    QEventLoop loop;
    auto grabResult = rootObject->grabToImage();
    QVERIFY(grabResult);
    loop.connect(grabResult.data(), &QQuickItemGrabResult::ready, &loop, &QEventLoop::quit);
    loop.exec();
    const QImage grabResultImage = grabResult->image();
    QVERIFY(!grabResultImage.isNull());
    QCOMPARE(grabResultImage.size(), sourceSize);
    QCOMPARE(grabResultImage.pixelColor(0, 0), expectedColorAtTopLeft);

    // Other checks
    // Check wallpaper interface
    QCOMPARE(rootObject->property("wallpaperInterface").value<MockWallpaperInterface *>(), wpGraphicObject);
    // Check item type
    QCOMPARE(evaluate<bool>(rootObject, "this instanceof QQC2.StackView"), true);
    QCOMPARE(evaluate<bool>(currentItem, "this instanceof Rectangle"), true);
    // Check item size
    QCOMPARE(m_view->rootObject()->size().toSize(), sourceSize);
}

QTEST_MAIN(ImageFrontendTest)

#include "tst_imagefrontend.moc"
