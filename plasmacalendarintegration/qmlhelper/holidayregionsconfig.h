/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QStringList>
#include <qqml.h>

#include <KConfigGroup>

class HolidayRegionsConfig : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QStringList selectedRegions READ selectedRegions NOTIFY selectedRegionsChanged)

public:
    explicit HolidayRegionsConfig(QObject *parent = nullptr);

    QStringList selectedRegions() const;
    Q_INVOKABLE void saveConfig();
    Q_INVOKABLE void addRegion(const QString &region);
    Q_INVOKABLE void removeRegion(const QString &region);

Q_SIGNALS:
    void selectedRegionsChanged();

private:
    QStringList m_regions;
    KConfigGroup m_configGroup;
};
