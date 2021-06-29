/*
    SPDX-FileCopyrightText: 2014 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2014 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "desktopview.h"
#include <plasma/corona.h>

namespace KActivities
{
class Consumer;
}

class StandaloneAppCorona : public Plasma::Corona
{
    Q_OBJECT

public:
    explicit StandaloneAppCorona(const QString &coronaPlugin, QObject *parent = nullptr);
    ~StandaloneAppCorona() override;

    QRect screenGeometry(int id) const override;

    void loadDefaultLayout() override;

    Plasma::Containment *createContainmentForActivity(const QString &activity, int screenNum);

    void insertActivity(const QString &id, const QString &plugin);
    Plasma::Containment *addPanel(const QString &plugin);

    Q_INVOKABLE QStringList availableActivities() const;

public Q_SLOTS:
    void load();

    void currentActivityChanged(const QString &newActivity);
    void activityAdded(const QString &id);
    void activityRemoved(const QString &id);
    void toggleWidgetExplorer();

protected Q_SLOTS:
    int screenForContainment(const Plasma::Containment *containment) const override;

private:
    QString m_coronaPlugin;
    KActivities::Consumer *m_activityConsumer;
    KConfigGroup m_desktopDefaultsConfig;
    DesktopView *m_view;
    QHash<QString, QString> m_activityContainmentPlugins;
};
