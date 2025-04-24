/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KCModuleData>

#include <QGeoCoordinate>
#include <QObject>

class StaticLocation
{
    Q_GADGET
    Q_PROPERTY(QGeoCoordinate coordinate MEMBER coordinate)
    Q_PROPERTY(bool present MEMBER present)

public:
    bool operator==(const StaticLocation &other) const = default;
    bool operator!=(const StaticLocation &other) const = default;

    QGeoCoordinate coordinate = QGeoCoordinate(0, 0, 0);
    bool present = false;
};

class GeoClueSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(StaticLocation staticLocation READ staticLocation WRITE setStaticLocation NOTIFY staticLocationChanged)

public:
    explicit GeoClueSettings(QObject *parent = nullptr);

    bool isDefaults() const;
    bool isSaveNeeded() const;

    bool isEnabled() const;
    void setEnabled(bool enabled);

    StaticLocation staticLocation() const;
    void setStaticLocation(const StaticLocation &location);

Q_SIGNALS:
    void enabledChanged();
    void staticLocationChanged();
    void changed();

public Q_SLOTS:
    void load();
    void save();
    void defaults();

private:
    struct {
        bool current = false;
        bool loaded = false;
        bool defaults = true;
    } m_enabled;

    struct {
        StaticLocation current;
        StaticLocation loaded;
        StaticLocation defaults;
    } m_staticLocation;
};

/*
class GeoClueSettings : public KConfigSkeleton
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(StaticLocation staticLocation READ staticLocation WRITE setStaticLocation NOTIFY staticLocationChanged)

public:
    explicit GeoClueSettings(QObject *parent = nullptr);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    StaticLocation staticLocation() const;
    void setStaticLocation(const StaticLocation &location);

Q_SIGNALS:
    void enabledChanged();
    void staticLocationChanged();

protected:
    void usrRead() override;
    bool usrSave() override;

private:
    void itemChanged(quint64 flag);

    StaticLocation m_staticLocation;
    bool m_enabled = false;
};

class GeoClueData : public KCModuleData
{
    Q_OBJECT

public:
    explicit GeoClueData(QObject *parent = nullptr);

    GeoClueSettings *settings() const;

private:
    GeoClueSettings *m_settings;
};

class EnabledItem : public KConfigSkeletonItem
{
public:
    EnabledItem(bool &location);

    void readConfig(KConfig *config) override;
    void writeConfig(KConfig *config) override;

    void readDefault(KConfig *config) override;
    void setDefault() override;
    void swapDefault() override;

    bool isEqual(const QVariant &p) const override;
    QVariant property() const override;
    void setProperty(const QVariant &p) override;

private:
    bool &m_reference;
    bool m_loaded;
    bool m_default;
};

class StaticLocationItem : public KConfigSkeletonItem
{
public:
    StaticLocationItem(StaticLocation &location);

    void readConfig(KConfig *config) override;
    void writeConfig(KConfig *config) override;

    void readDefault(KConfig *config) override;
    void setDefault() override;
    void swapDefault() override;

    bool isEqual(const QVariant &p) const override;
    QVariant property() const override;
    void setProperty(const QVariant &p) override;

private:
    StaticLocation &m_reference;
    StaticLocation m_loaded;
    StaticLocation m_default;
};
*/
