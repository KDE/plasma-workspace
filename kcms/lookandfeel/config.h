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

public:
    explicit LookAndFeelConfigStore(LookAndFeelSettings *settings, QObject *parent = nullptr);

    QString lookAndFeelPackage() const;
    void setLookAndFeelPackage(const QString &package);

    QString defaultLightLookAndFeel() const;
    void setDefaultLightLookAndFeel(const QString &package);

    QString defaultDarkLookAndFeel() const;
    void setDefaultDarkLookAndFeel(const QString &package);

Q_SIGNALS:
    void lookAndFeelPackageChanged();
    void defaultLightLookAndFeelChanged();
    void defaultDarkLookAndFeelChanged();

private:
    QString m_lookAndFeelPackage;
    QString m_defaultLightLookAndFeel;
    QString m_defaultDarkLookAndFeel;
};

class LookAndFeelConfig : public KCoreConfigSkeleton
{
    Q_OBJECT
    Q_PROPERTY(QString lookAndFeelPackage READ lookAndFeelPackage WRITE setLookAndFeelPackage NOTIFY lookAndFeelPackageChanged)
    Q_PROPERTY(QString defaultLightLookAndFeel READ defaultLightLookAndFeel WRITE setDefaultLightLookAndFeel NOTIFY defaultLightLookAndFeelChanged)
    Q_PROPERTY(QString defaultDarkLookAndFeel READ defaultDarkLookAndFeel WRITE setDefaultDarkLookAndFeel NOTIFY defaultDarkLookAndFeelChanged)

public:
    explicit LookAndFeelConfig(LookAndFeelSettings *settings, QObject *parent = nullptr);

    QString lookAndFeelPackage() const;
    void setLookAndFeelPackage(const QString &package);

    QString defaultLightLookAndFeel() const;
    void setDefaultLightLookAndFeel(const QString &package);

    QString defaultDarkLookAndFeel() const;
    void setDefaultDarkLookAndFeel(const QString &package);

Q_SIGNALS:
    void lookAndFeelPackageChanged();
    void defaultLightLookAndFeelChanged();
    void defaultDarkLookAndFeelChanged();

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
};
