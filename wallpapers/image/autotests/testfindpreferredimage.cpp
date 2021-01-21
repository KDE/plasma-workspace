/***************************************************************************
 *   Copyright 2016 Antonio Larrosa <larrosa@kde.org>                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/
#include "image.h"
#include <QDebug>
#include <QtTest>

extern QSize resSize(const QString &str);

QString formatResolution(const QString &str)
{
    QSize size = resSize(str);
    float aspectRatio = (size.height() > 0) ? size.width() / (float)size.height() : 0;
    return QStringLiteral("%1 (%2)").arg(str, 9).arg(aspectRatio, 7);
}

class TestResolutions : public QObject
{
    Q_OBJECT

private slots:
    void testResolutions_data();
    void testResolutions();

protected:
    Image m_image;
    QStringList m_images;
};

void TestResolutions::testResolutions_data()
{
    // The list of available wallpaper image sizes
    m_images << QStringLiteral("1280x1024") << QStringLiteral("1350x1080") << QStringLiteral("1440x1080") << QStringLiteral("1600x1200")
             << QStringLiteral("1920x1080") << QStringLiteral("1920x1200") << QStringLiteral("3840x2400");
    qDebug() << "Available images:";
    foreach (auto image, m_images) {
        qDebug() << formatResolution(image);
    }

    // The list of possible screen resolutions to test and the appropriate images that should be chosen
    QTest::addColumn<QString>("resolution");
    QTest::addColumn<QString>("expected");
    QTest::newRow("1280x1024") << QStringLiteral("1280x1024") << QStringLiteral("1280x1024");
    QTest::newRow("1350x1080") << QStringLiteral("1350x1080") << QStringLiteral("1350x1080");
    QTest::newRow("1440x1080") << QStringLiteral("1440x1080") << QStringLiteral("1440x1080");
    QTest::newRow("1600x1200") << QStringLiteral("1600x1200") << QStringLiteral("1600x1200");
    QTest::newRow("1920x1080") << QStringLiteral("1920x1080") << QStringLiteral("1920x1080");
    QTest::newRow("1920x1200") << QStringLiteral("1920x1200") << QStringLiteral("1920x1200");
    QTest::newRow("3840x2400") << QStringLiteral("3840x2400") << QStringLiteral("3840x2400");
    QTest::newRow("4096x2160") << QStringLiteral("4096x2160") << QStringLiteral("1920x1080");
    QTest::newRow("3840x2160") << QStringLiteral("3840x2160") << QStringLiteral("1920x1080");
    QTest::newRow("3200x1800") << QStringLiteral("3200x1800") << QStringLiteral("1920x1080");
    QTest::newRow("2048x1080") << QStringLiteral("2048x1080") << QStringLiteral("1920x1080");
    QTest::newRow("1680x1050") << QStringLiteral("1680x1050") << QStringLiteral("1920x1200");
    QTest::newRow("1400x1050") << QStringLiteral("1400x1050") << QStringLiteral("1440x1080");
    QTest::newRow("1440x900") << QStringLiteral("1440x900") << QStringLiteral("1920x1200");
    QTest::newRow("1280x960") << QStringLiteral("1280x960") << QStringLiteral("1440x1080");
    QTest::newRow("1280x854") << QStringLiteral("1280x854") << QStringLiteral("1920x1200");
    QTest::newRow("1280x800") << QStringLiteral("1280x800") << QStringLiteral("1920x1200");
    QTest::newRow("1280x720") << QStringLiteral("1280x720") << QStringLiteral("1920x1080");
    QTest::newRow("1152x768") << QStringLiteral("1152x768") << QStringLiteral("1920x1200");
    QTest::newRow("1024x768") << QStringLiteral("1024x768") << QStringLiteral("1440x1080");
    QTest::newRow("800x600") << QStringLiteral("800x600") << QStringLiteral("1440x1080");
    QTest::newRow("848x480") << QStringLiteral("848x480") << QStringLiteral("1920x1080");
    QTest::newRow("720x480") << QStringLiteral("720x480") << QStringLiteral("1920x1200");
    QTest::newRow("640x480") << QStringLiteral("640x480") << QStringLiteral("1440x1080");
    QTest::newRow("1366x768") << QStringLiteral("1366x768") << QStringLiteral("1920x1080");
    QTest::newRow("1600x814") << QStringLiteral("1600x814") << QStringLiteral("1920x1080");
}

void TestResolutions::testResolutions()
{
    QFETCH(QString, resolution);
    QFETCH(QString, expected);

    m_image.setTargetSize(resSize(resolution));
    QString preferred = m_image.findPreferedImage(m_images);

    qDebug() << "For a screen size of " << formatResolution(resolution) << " the " << formatResolution(preferred) << " wallpaper was preferred";

    QCOMPARE(preferred, expected);
}

QTEST_MAIN(TestResolutions)
#include "testfindpreferredimage.moc"
