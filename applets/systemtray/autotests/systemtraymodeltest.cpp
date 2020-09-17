/*
 * Copyright 2020  Konrad Materka <materka@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtTest>

#include <Plasma/Applet>
#include <Plasma/DataEngine>
#include <Plasma/PluginLoader>

#include "../systemtraymodel.h"

class SystemTrayModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void testPlasmoidModel();
};

void SystemTrayModelTest::init()
{
    QLocale::setDefault(QLocale("en_US"));
    qunsetenv("LANGUAGE");
    qunsetenv("LC_ALL");
    qunsetenv("LC_MESSAGES");
    qunsetenv("LANG");
}

void SystemTrayModelTest::testPlasmoidModel()
{
    //given: sample plugin meta data
    QList<KPluginMetaData> list;
    list.append(KPluginMetaData(QFINDTESTDATA("data/mediacontroller/metadata.desktop")));
    list.append(KPluginMetaData(QFINDTESTDATA("data/devicenotifier/metadata.desktop")));

    //when: model is initialized
    PlasmoidModel *model = new PlasmoidModel();
    model->init(list);

    //expect: passes consistency tests
    new QAbstractItemModelTester(model, QAbstractItemModelTester::FailureReportingMode::Fatal);

    //and expect: correct model size
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->roleNames().size(), 10);
    //and expect: correct data returned
    QModelIndex idx = model->index(0, 0);
    QCOMPARE(model->data(idx, Qt::DisplayRole).toString(), "Media Player (Automatic load)");
    QVERIFY(model->data(idx, Qt::DecorationRole).isValid());
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::ItemType)).toString(), "Plasmoid");
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::ItemId)).toString(), "org.kde.plasma.mediacontroller.test");
    QVERIFY(!model->data(idx, static_cast<int>(BaseModel::BaseRole::CanRender)).toBool());
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::Category)).toString(), "ApplicationStatus");
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::Status)), QVariant(Plasma::Types::ItemStatus::UnknownStatus));
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::EffectiveStatus)), QVariant(Plasma::Types::ItemStatus::HiddenStatus));
    QVERIFY(!model->data(idx, static_cast<int>(PlasmoidModel::Role::HasApplet)).toBool());
    idx = model->index(1, 0);
    QCOMPARE(model->data(idx, Qt::DisplayRole).toString(), "Device Notifier");
    QVERIFY(model->data(idx, Qt::DecorationRole).isValid());
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::ItemType)).toString(), "Plasmoid");
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::ItemId)).toString(), "org.kde.plasma.devicenotifier.test");
    QVERIFY(!model->data(idx, static_cast<int>(BaseModel::BaseRole::CanRender)).toBool());
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::Category)).toString(), "Hardware");
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::Status)), QVariant(Plasma::Types::ItemStatus::UnknownStatus));
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::EffectiveStatus)), QVariant(Plasma::Types::ItemStatus::HiddenStatus));
    QVERIFY(!model->data(idx, static_cast<int>(PlasmoidModel::Role::HasApplet)).toBool());

    //when: language is changed
    QLocale::setDefault(QLocale("pl_PL"));
    qputenv("LANG", "pl_PL.UTF-8");
    qputenv("LC_MESSAGES", "pl_PL.UTF-8");
    //then expect: translated data returned
    QCOMPARE(model->data(idx, Qt::DisplayRole).toString(), "Powiadomienia o urz\u0105dzeniach");

    //when: applet added
    model->addApplet(new Plasma::Applet(list.at(1)));
    //then: applet can be rendered
    QVERIFY(model->data(idx, static_cast<int>(BaseModel::BaseRole::CanRender)).toBool());
    QVERIFY(model->data(idx, static_cast<int>(PlasmoidModel::Role::HasApplet)).toBool());
    QCOMPARE(model->data(idx, static_cast<int>(BaseModel::BaseRole::EffectiveStatus)), QVariant(Plasma::Types::ItemStatus::ActiveStatus));

    //and when: applet removed
    model->removeApplet(new Plasma::Applet(list.at(1)));
    //then: applet cannot be rendered anymore
    QVERIFY(!model->data(idx, static_cast<int>(BaseModel::BaseRole::CanRender)).toBool());
    QVERIFY(!model->data(idx, static_cast<int>(PlasmoidModel::Role::HasApplet)).toBool());

    //and when: invalid index
    idx = model->index(4, 0);
    //then: empty value
    QVERIFY(model->data(idx, static_cast<int>(BaseModel::BaseRole::ItemType)).isNull());
    QVERIFY(!model->data(idx, static_cast<int>(BaseModel::BaseRole::ItemType)).isValid());
    idx = model->index(1, 1);
    QVERIFY(model->data(idx, static_cast<int>(BaseModel::BaseRole::ItemType)).isNull());
    QVERIFY(!model->data(idx, static_cast<int>(BaseModel::BaseRole::ItemType)).isValid());

    delete model;
}

QTEST_MAIN(SystemTrayModelTest)

#include "systemtraymodeltest.moc"
