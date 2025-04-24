/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KCModuleData>
#include <KConfigSkeleton>

#include <QGeoCoordinate>
#include <QObject>

class ManualLocation
{
    Q_GADGET
    Q_PROPERTY(QGeoCoordinate coordinate MEMBER coordinate)
    Q_PROPERTY(bool present MEMBER present)

public:
    bool operator==(const ManualLocation &other) const = default;
    bool operator!=(const ManualLocation &other) const = default;

    QGeoCoordinate coordinate = QGeoCoordinate(0, 0, 0);
    bool present = false;
};

class ManualLocationItem : public KConfigSkeletonItem
{
public:
    ManualLocationItem(ManualLocation &location);

    void readConfig(KConfig *config) override;
    void writeConfig(KConfig *config) override;

    void readDefault(KConfig *config) override;
    void setDefault() override;
    void swapDefault() override;

    bool isEqual(const QVariant &p) const override;
    QVariant property() const override;
    void setProperty(const QVariant &p) override;

private:
    ManualLocation &m_reference;
    ManualLocation m_loaded;
    ManualLocation m_default;
};

class LocationSettings : public KConfigSkeleton
{
    Q_OBJECT
    Q_PROPERTY(ManualLocation manualLocation READ manualLocation WRITE setManualLocation NOTIFY manualLocationChanged)

public:
    explicit LocationSettings(QObject *parent = nullptr);

    ManualLocation manualLocation() const;
    void setManualLocation(const ManualLocation &location);

Q_SIGNALS:
    void manualLocationChanged();

private:
    void itemChanged(quint64 flag);

    ManualLocation m_manualLocation;
};

class LocationData : public KCModuleData
{
    Q_OBJECT

public:
    explicit LocationData(QObject *parent = nullptr);

    LocationSettings *settings() const;

private:
    LocationSettings *m_settings;
};
