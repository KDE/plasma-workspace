// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include <QDebug>
#include <QTest>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

#include <kxftconfig.h>

class KXftConfigTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testLoad()
    {
        KXftConfig c(QFINDTESTDATA("fonts.conf"));

        KXftConfig::SubPixel::Type subPixelType = KXftConfig::SubPixel::NotSet;
        QVERIFY(c.getSubPixelType(subPixelType));
        QCOMPARE(subPixelType, KXftConfig::SubPixel::Vbgr);
        QVERIFY(c.subPixelTypeHasLocalConfig());

        KXftConfig::Hint::Style hintStyle = KXftConfig::Hint::NotSet;
        QVERIFY(c.getHintStyle(hintStyle));
        QCOMPARE(hintStyle, KXftConfig::Hint::Full);
        QVERIFY(c.hintStyleHasLocalConfig());

        QVERIFY(c.aliasingEnabled());
        QVERIFY(c.getAntiAliasing());
        QVERIFY(c.antiAliasingHasLocalConfig());

        double from = -1.0;
        double to = -1.0;
        QVERIFY(c.getExcludeRange(from, to));
        QCOMPARE(from, 8);
        QCOMPARE(to, 15);

        const int appDpiY = QX11Info::appDpiY();
        qDebug() << "QX11Info::appDpiY()" << appDpiY;
        if (appDpiY == 96) {
            QVERIFY(!c.changed());
        } else {
            // pointSize and pixelSize do not match
            QVERIFY(c.changed());
        }
    }
};

QTEST_MAIN(KXftConfigTest)

#include "kxftconfigtest.moc"
