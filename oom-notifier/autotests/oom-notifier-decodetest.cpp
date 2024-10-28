// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

#include "../decode.h"

using namespace Qt::StringLiterals;

class DecodeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testNoDecode()
    {
        auto input = u"/org/freedesktop/systemd1/unit/app-org.kde.konsole-7cb071a4bc6d4115811586ff8b527aac.scope"_s;
        auto output = decodeUnitName(input);
        QCOMPARE(output, input);
    }

    void testDecode()
    {
        auto input = u"/org/freedesktop/systemd1/unit/app_2dorg_2ekde_2ekonsole_2d7cb071a4bc6d4115811586ff8b527aac_2escope"_s;
        auto output = decodeUnitName(input);
        QCOMPARE(output, "/org/freedesktop/systemd1/unit/app-org.kde.konsole-7cb071a4bc6d4115811586ff8b527aac.scope");
    }

    void testUnitNameGarbage()
    {
        auto output = unitNameToServiceName(u"foobar"_s);
        QCOMPARE(output, QString());
    }

    void testUnitNameToServiceName()
    {
        auto output = unitNameToServiceName(u"app-org.kde.konsole-7cb071a4bc6d4115811586ff8b527aac.scope"_s);
        QCOMPARE(output, u"org.kde.konsole"_s);
    }

    void testUnitNameWithoutSuffix()
    {
        auto output = unitNameToServiceName(u"app-foobar"_s);
        QCOMPARE(output, u"foobar"_s);
    }

    void testFlatpakUnitNameToServiceName()
    {
        auto output = unitNameToServiceName(u"app-flatpak-org.kde.konsole-7cb071a4bc6d4115811586ff8b527aac.scope"_s);
        QCOMPARE(output, u"org.kde.konsole"_s);
    }

    void testUnitNameToServiceNameAtSuffix()
    {
        auto output = unitNameToServiceName(u"app-flatpak-org.kde.konsole@7cb071a4bc6d4115811586ff8b527aac.scope"_s);
        QCOMPARE(output, u"org.kde.konsole"_s);
    }
};

QTEST_GUILESS_MAIN(DecodeTest)

#include "oom-notifier-decodetest.moc"
