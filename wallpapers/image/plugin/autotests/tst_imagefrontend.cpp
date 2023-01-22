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

#include <KPackage/PackageLoader>

#include "../utils/mediaproxy.h"
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
        connect(this, &MockWallpaperInterface::repaintNeeded, this, [this](const QColor &color) {
            m_repainted = true;
            m_accentColor = color;
        });
    }

    bool m_repainted = false; // To be set to true by QQC2.StackView.onActivated
    bool m_loading = true; // To be set to false by replaceWhenLoaded
    QColor m_accentColor = Qt::transparent;

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
    void testReloadWallpaperOnScreenSizeChanged();

    void testCustomAccentColorFromWallpaperMetaData();

private:
    QPointer<QQuickView> m_view;
    QPointer<MockWallpaperInterface> m_wallpaperInterface;

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
    Q_ASSERT(!m_view && !m_wallpaperInterface);
    m_view = new QQuickView();
    m_view->engine()->setBaseUrl(QUrl::fromLocalFile(QFINDTESTDATA("../../imagepackage/contents/ui/")));
    m_wallpaperInterface = new MockWallpaperInterface(m_view);
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
    initialProperties.insert(QStringLiteral("wallpaperInterface"), QVariant::fromValue(m_wallpaperInterface.data()));
    m_view->setInitialProperties(initialProperties);

    // When repaintNeeded is emitted, the transition animation has finished.
    QSignalSpy repaintSpy(m_wallpaperInterface, &MockWallpaperInterface::repaintNeeded);

    QByteArray errorMessage;
    QVERIFY2(initView(m_view.data(), QUrl::fromLocalFile(QFINDTESTDATA("../../imagepackage/contents/ui/ImageStackView.qml")), &errorMessage),
             errorMessage.constData());

    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view));
    QQuickItem *rootObject = m_view->rootObject();
    QVERIFY(rootObject);

    // Wait loaded
    QVERIFY(m_wallpaperInterface->m_repainted || repaintSpy.wait());
    auto currentItem = evaluate<QQuickItem *>(rootObject, "currentItem");
    QVERIFY(currentItem);

    // Check required properties are set correctly
    QCOMPARE(evaluate<int>(rootObject, "fillMode"), fillMode);
    QCOMPARE(evaluate<int>(currentItem, "fillMode"), fillMode);
    QCOMPARE(evaluate<QString>(rootObject, "configColor"), configColor);
    QCOMPARE(evaluate<QColor>(currentItem, "color"), QColor(configColor));
    QCOMPARE(evaluate<bool>(rootObject, "blur"), blur);
    QCOMPARE(evaluate<bool>(currentItem, "blur"), blur);
    QCOMPARE(evaluate<QString>(rootObject, "source"), QUrl::fromUserInput(source).toString());
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
    QCOMPARE(rootObject->property("wallpaperInterface").value<MockWallpaperInterface *>(), m_wallpaperInterface.data());
    // Check item type
    QCOMPARE(evaluate<bool>(rootObject, "this instanceof QQC2.StackView"), true);
    QCOMPARE(evaluate<bool>(currentItem, "this instanceof Rectangle"), true);
    // Check item size
    QCOMPARE(m_view->rootObject()->size().toSize(), sourceSize);
}

void ImageFrontendTest::testReloadWallpaperOnScreenSizeChanged()
{
    // Set required properties and window size
    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("fillMode"), 1 /* PreserveAspectFit */);
    initialProperties.insert(QStringLiteral("configColor"), QStringLiteral("black"));
    initialProperties.insert(QStringLiteral("blur"), false);
    // Set a package that contains different resolutions
    initialProperties.insert(QStringLiteral("source"), m_dataDir.absoluteFilePath(ImageBackendTestData::defaultPackageFolderName2));
    QSize sourceSize(1024, 768);
    initialProperties.insert(QStringLiteral("sourceSize"), sourceSize);
    initialProperties.insert(QStringLiteral("width"), sourceSize.width());
    initialProperties.insert(QStringLiteral("height"), sourceSize.height());
    initialProperties.insert(QStringLiteral("wallpaperInterface"), QVariant::fromValue(m_wallpaperInterface.data()));
    m_view->setInitialProperties(initialProperties);

    // When repaintNeeded is emitted, the transition animation has finished.
    QSignalSpy repaintSpy(m_wallpaperInterface, &MockWallpaperInterface::repaintNeeded);

    QByteArray errorMessage;
    QVERIFY2(initView(m_view.data(), QUrl::fromLocalFile(QFINDTESTDATA("../../imagepackage/contents/ui/ImageStackView.qml")), &errorMessage),
             errorMessage.constData());

    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view));
    QQuickItem *rootObject = m_view->rootObject();
    QVERIFY(rootObject);

    // Wait loaded
    QVERIFY(m_wallpaperInterface->m_repainted || repaintSpy.wait());
    auto firstItem = evaluate<QQuickItem *>(rootObject, "currentItem");
    QVERIFY(firstItem);
    QSignalSpy firstItemDestroySpy(firstItem, &QObject::destroyed);

    // Case 1: 1024x768
    QUrl source = evaluate<QUrl>(firstItem, "source");
    QVERIFY(source.toString().contains(QLatin1String("targetWidth=1024&targetHeight=768")));
    QCOMPARE(evaluate<QSizeF>(firstItem, "sourceSize"), QSizeF(1024, 768));

    // Change sourceSize
    evaluate<void>(rootObject, "sourceSize = Qt.size(1920, 1080);");
    // Qt.callLater
    QVERIFY(repaintSpy.wait());
    // The first item should be destroyed, otherwise there will be a memory leak
    QVERIFY(firstItemDestroySpy.wait());
    auto secondItem = evaluate<QQuickItem *>(rootObject, "currentItem");
    QVERIFY(secondItem);
    QSignalSpy secondItemDestroySpy(secondItem, &QObject::destroyed);
    // Case 2: 1920x1080
    source = evaluate<QUrl>(secondItem, "source");
    QVERIFY(source.toString().contains(QLatin1String("targetWidth=1920&targetHeight=1080")));
    QCOMPARE(evaluate<QSizeF>(secondItem, "sourceSize"), QSizeF(1920, 1080));

    // Now set a single image to test if the frontend will still reload the wallpaper in different sizes
    const QString singleImagePath = m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1);
    QVERIFY(QFileInfo::exists(singleImagePath));
    rootObject->setProperty("source", singleImagePath);
    rootObject->setProperty("sourceSize", QSizeF(320, 240));
    // Qt.callLater
    QVERIFY(repaintSpy.wait());
    // The second item should also be destroyed, otherwise there will be a memory leak
    QVERIFY(secondItemDestroySpy.wait());
    auto thirdItem = evaluate<QQuickItem *>(rootObject, "currentItem");
    QVERIFY(thirdItem);
    QSignalSpy thirdItemDestroySpy(thirdItem, &QObject::destroyed);
    source = evaluate<QUrl>(thirdItem, "source");
    QCOMPARE(source, QUrl::fromLocalFile(singleImagePath));
    QCOMPARE(evaluate<QSizeF>(thirdItem, "sourceSize"), QSizeF(320, 240));

    // Now change to 1920x1080
    evaluate<void>(rootObject, "sourceSize = Qt.size(1920, 1080);");
    // Qt.callLater
    QVERIFY(repaintSpy.wait());
    // The third item should also be destroyed, otherwise there will be a memory leak
    QVERIFY(thirdItemDestroySpy.wait());
    auto fourthItem = evaluate<QQuickItem *>(rootObject, "currentItem");
    QVERIFY(fourthItem);
    source = evaluate<QUrl>(fourthItem, "source");
    QCOMPARE(source, QUrl::fromLocalFile(singleImagePath));
    QCOMPARE(evaluate<QSizeF>(fourthItem, "sourceSize"), QSizeF(1920, 1080));
}

void ImageFrontendTest::testCustomAccentColorFromWallpaperMetaData()
{
    // Case 1: value is a dict
    auto package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(QFINDTESTDATA(ImageBackendTestData::customAccentColorPackage1));
    QVERIFY(package.isValid());

    QPalette palette;
    // Light variant
    palette.setColor(QPalette::Normal, QPalette::Window, Qt::white);
    qGuiApp->setPalette(palette);
    QColor customColor = MediaProxy::getAccentColorFromMetaData(package);
    QCOMPARE(customColor, Qt::red);

    // Dark variant
    palette.setColor(QPalette::Normal, QPalette::Window, Qt::black);
    qGuiApp->setPalette(palette);
    customColor = MediaProxy::getAccentColorFromMetaData(package);
    QCOMPARE(customColor, Qt::cyan);

    // Case 2: value is a string
    package.setPath(QFINDTESTDATA(ImageBackendTestData::customAccentColorPackage2));
    QVERIFY(package.isValid());
    customColor = MediaProxy::getAccentColorFromMetaData(package);
    QCOMPARE(customColor, QColor("green")); // Qt::green is not QColor("green")

    // Real-life test
    palette.setColor(QPalette::Normal, QPalette::Window, Qt::white);
    qGuiApp->setPalette(palette);
    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("fillMode"), 1 /* PreserveAspectFit */);
    initialProperties.insert(QStringLiteral("configColor"), QStringLiteral("black"));
    initialProperties.insert(QStringLiteral("blur"), false);
    initialProperties.insert(QStringLiteral("source"), QFINDTESTDATA(ImageBackendTestData::customAccentColorPackage1));
    QSize sourceSize(320, 240);
    initialProperties.insert(QStringLiteral("sourceSize"), sourceSize);
    initialProperties.insert(QStringLiteral("width"), sourceSize.width());
    initialProperties.insert(QStringLiteral("height"), sourceSize.height());
    initialProperties.insert(QStringLiteral("wallpaperInterface"), QVariant::fromValue(m_wallpaperInterface.data()));
    m_view->setInitialProperties(initialProperties);

    QSignalSpy repaintSpy(m_wallpaperInterface, &MockWallpaperInterface::repaintNeeded);

    QByteArray errorMessage;
    QVERIFY2(initView(m_view.data(), QUrl::fromLocalFile(QFINDTESTDATA("../../imagepackage/contents/ui/ImageStackView.qml")), &errorMessage),
             errorMessage.constData());

    m_view->show();
    QVERIFY(repaintSpy.wait());
    QCOMPARE(m_wallpaperInterface->m_accentColor, Qt::red);

    palette.setColor(QPalette::Normal, QPalette::Window, Qt::black);
    qGuiApp->setPalette(palette);
    QVERIFY(repaintSpy.wait());
    QCOMPARE(m_wallpaperInterface->m_accentColor, Qt::cyan);

    // Switch to a wallpaper package that does not contain accent color information
    m_view->rootObject()->setProperty("source", m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName1));
    QVERIFY(repaintSpy.wait());
    QCOMPARE(m_wallpaperInterface->m_accentColor, Qt::transparent);
}

QTEST_MAIN(ImageFrontendTest)

#include "tst_imagefrontend.moc"
