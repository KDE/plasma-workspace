/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "lookandfeelsettings.h"

class LookAndFeelConfigStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lookAndFeelPackage READ lookAndFeelPackage WRITE setLookAndFeelPackage NOTIFY lookAndFeelPackageChanged)
    Q_PROPERTY(QString defaultLightLookAndFeel READ defaultLightLookAndFeel WRITE setDefaultLightLookAndFeel NOTIFY defaultLightLookAndFeelChanged)
    Q_PROPERTY(QString defaultDarkLookAndFeel READ defaultDarkLookAndFeel WRITE setDefaultDarkLookAndFeel NOTIFY defaultDarkLookAndFeelChanged)
    Q_PROPERTY(bool automaticLookAndFeel READ automaticLookAndFeel WRITE setAutomaticLookAndFeel NOTIFY automaticLookAndFeelChanged)
    Q_PROPERTY(bool automaticLookAndFeelOnIdle READ automaticLookAndFeelOnIdle WRITE setAutomaticLookAndFeelOnIdle NOTIFY automaticLookAndFeelOnIdleChanged)
    Q_PROPERTY(uint automaticLookAndFeelIdleInterval READ automaticLookAndFeelIdleInterval WRITE setAutomaticLookAndFeelIdleInterval NOTIFY automaticLookAndFeelIdleIntervalChanged)

public:
    explicit LookAndFeelConfigStore(LookAndFeelSettings *settings, QObject *parent = nullptr);

    QString lookAndFeelPackage() const;
    void setLookAndFeelPackage(const QString &package);

    QString defaultLightLookAndFeel() const;
    void setDefaultLightLookAndFeel(const QString &package);

    QString defaultDarkLookAndFeel() const;
    void setDefaultDarkLookAndFeel(const QString &package);

    bool automaticLookAndFeel() const;
    void setAutomaticLookAndFeel(bool set);

    bool automaticLookAndFeelOnIdle() const;
    void setAutomaticLookAndFeelOnIdle(bool set);

    uint automaticLookAndFeelIdleInterval() const;
    void setAutomaticLookAndFeelIdleInterval(uint interval);

Q_SIGNALS:
    void lookAndFeelPackageChanged();
    void defaultLightLookAndFeelChanged();
    void defaultDarkLookAndFeelChanged();
    void automaticLookAndFeelChanged();
    void automaticLookAndFeelOnIdleChanged();
    void automaticLookAndFeelIdleIntervalChanged();

private:
    QString m_lookAndFeelPackage;
    QString m_defaultLightLookAndFeel;
    QString m_defaultDarkLookAndFeel;
    bool m_automaticLookAndFeel = false;
    bool m_automaticLookAndFeelOnIdle = true;
    uint m_automaticLookAndFeelIdleInterval = 1;
};

class LookAndFeelConfig : public KCoreConfigSkeleton
{
    Q_OBJECT
    Q_PROPERTY(QString lookAndFeelPackage READ lookAndFeelPackage WRITE setLookAndFeelPackage NOTIFY lookAndFeelPackageChanged)
    Q_PROPERTY(QString defaultLightLookAndFeel READ defaultLightLookAndFeel WRITE setDefaultLightLookAndFeel NOTIFY defaultLightLookAndFeelChanged)
    Q_PROPERTY(QString defaultDarkLookAndFeel READ defaultDarkLookAndFeel WRITE setDefaultDarkLookAndFeel NOTIFY defaultDarkLookAndFeelChanged)
    Q_PROPERTY(bool automaticLookAndFeel READ automaticLookAndFeel WRITE setAutomaticLookAndFeel NOTIFY automaticLookAndFeelChanged)
    Q_PROPERTY(bool automaticLookAndFeelOnIdle READ automaticLookAndFeelOnIdle WRITE setAutomaticLookAndFeelOnIdle NOTIFY automaticLookAndFeelOnIdleChanged)
    Q_PROPERTY(uint automaticLookAndFeelIdleInterval READ automaticLookAndFeelIdleInterval WRITE setAutomaticLookAndFeelIdleInterval NOTIFY automaticLookAndFeelIdleIntervalChanged)

public:
    explicit LookAndFeelConfig(LookAndFeelSettings *settings, QObject *parent = nullptr);

    QString lookAndFeelPackage() const;
    void setLookAndFeelPackage(const QString &package);

    QString defaultLightLookAndFeel() const;
    void setDefaultLightLookAndFeel(const QString &package);

    QString defaultDarkLookAndFeel() const;
    void setDefaultDarkLookAndFeel(const QString &package);

    bool automaticLookAndFeel() const;
    void setAutomaticLookAndFeel(bool set);

    bool automaticLookAndFeelOnIdle() const;
    void setAutomaticLookAndFeelOnIdle(bool set);

    uint automaticLookAndFeelIdleInterval() const;
    void setAutomaticLookAndFeelIdleInterval(uint interval);

Q_SIGNALS:
    void lookAndFeelPackageChanged();
    void defaultLightLookAndFeelChanged();
    void defaultDarkLookAndFeelChanged();
    void automaticLookAndFeelChanged();
    void automaticLookAndFeelOnIdleChanged();
    void automaticLookAndFeelIdleIntervalChanged();

protected:
    bool usrSave() override;

private:
    using NotifySignalType = void (LookAndFeelConfig::*)();
    KPropertySkeletonItem *addItemInternal(const QByteArray &propertyName, const QVariant &defaultValue, NotifySignalType notifySignal);

    LookAndFeelSettings *m_settings;
    LookAndFeelConfigStore m_store;

    KPropertySkeletonItem *m_lookAndFeelPackageItem = nullptr;
    KPropertySkeletonItem *m_defaultLightLookAndFeelItem = nullptr;
    KPropertySkeletonItem *m_defaultDarkLookAndFeelItem = nullptr;
    KPropertySkeletonItem *m_automaticLookAndFeelItem = nullptr;
    KPropertySkeletonItem *m_automaticLookAndFeelOnIdleItem = nullptr;
    KPropertySkeletonItem *m_automaticLookAndFeelIdleIntervalItem = nullptr;
};
