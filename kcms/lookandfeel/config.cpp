/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "config.h"

LookAndFeelConfigStore::LookAndFeelConfigStore(LookAndFeelSettings *settings, QObject *parent)
    : QObject(parent)
    , m_lookAndFeelPackage(settings->lookAndFeelPackage())
    , m_defaultLightLookAndFeel(settings->defaultLightLookAndFeel())
    , m_defaultDarkLookAndFeel(settings->defaultDarkLookAndFeel())
{
}

QString LookAndFeelConfigStore::lookAndFeelPackage() const
{
    return m_lookAndFeelPackage;
}

void LookAndFeelConfigStore::setLookAndFeelPackage(const QString &package)
{
    if (m_lookAndFeelPackage != package) {
        m_lookAndFeelPackage = package;
        Q_EMIT lookAndFeelPackageChanged();
    }
}

QString LookAndFeelConfigStore::defaultLightLookAndFeel() const
{
    return m_defaultLightLookAndFeel;
}

void LookAndFeelConfigStore::setDefaultLightLookAndFeel(const QString &package)
{
    if (m_defaultLightLookAndFeel != package) {
        m_defaultLightLookAndFeel = package;
        Q_EMIT defaultLightLookAndFeelChanged();
    }
}

QString LookAndFeelConfigStore::defaultDarkLookAndFeel() const
{
    return m_defaultDarkLookAndFeel;
}

void LookAndFeelConfigStore::setDefaultDarkLookAndFeel(const QString &package)
{
    if (m_defaultDarkLookAndFeel != package) {
        m_defaultDarkLookAndFeel = package;
        Q_EMIT defaultDarkLookAndFeelChanged();
    }
}

LookAndFeelConfig::LookAndFeelConfig(LookAndFeelSettings *settings, QObject *parent)
    : KCoreConfigSkeleton(QString(), parent)
    , m_settings(settings)
    , m_store(settings)
{
    m_lookAndFeelPackageItem =
        addItemInternal(QByteArrayLiteral("lookAndFeelPackage"), m_settings->defaultLookAndFeelPackageValue(), &LookAndFeelConfig::lookAndFeelPackageChanged);
    m_defaultLightLookAndFeelItem = addItemInternal(QByteArrayLiteral("defaultLightLookAndFeel"),
                                                    m_settings->defaultDefaultLightLookAndFeelValue(),
                                                    &LookAndFeelConfig::defaultLightLookAndFeelChanged);
    m_defaultDarkLookAndFeelItem = addItemInternal(QByteArrayLiteral("defaultDarkLookAndFeel"),
                                                   m_settings->defaultDefaultDarkLookAndFeelValue(),
                                                   &LookAndFeelConfig::defaultDarkLookAndFeelChanged);
}

KPropertySkeletonItem *LookAndFeelConfig::addItemInternal(const QByteArray &propertyName, const QVariant &defaultValue, NotifySignalType notifySignal)
{
    auto item = new KPropertySkeletonItem(&m_store, propertyName, defaultValue);
    addItem(item, QString::fromLatin1(propertyName));
    item->setNotifyFunction([this, notifySignal] {
        Q_EMIT(this->*notifySignal)();
    });

    return item;
}

QString LookAndFeelConfig::lookAndFeelPackage() const
{
    return m_lookAndFeelPackageItem->property().toString();
}

void LookAndFeelConfig::setLookAndFeelPackage(const QString &package)
{
    m_lookAndFeelPackageItem->setProperty(package);
}

QString LookAndFeelConfig::defaultLightLookAndFeel() const
{
    return m_defaultLightLookAndFeelItem->property().toString();
}

void LookAndFeelConfig::setDefaultLightLookAndFeel(const QString &package)
{
    m_defaultLightLookAndFeelItem->setProperty(package);
}

QString LookAndFeelConfig::defaultDarkLookAndFeel() const
{
    return m_defaultDarkLookAndFeelItem->property().toString();
}

void LookAndFeelConfig::setDefaultDarkLookAndFeel(const QString &package)
{
    m_defaultDarkLookAndFeelItem->setProperty(package);
}

bool LookAndFeelConfig::usrSave()
{
    m_settings->setLookAndFeelPackage(lookAndFeelPackage());
    m_settings->setDefaultLightLookAndFeel(defaultLightLookAndFeel());
    m_settings->setDefaultDarkLookAndFeel(defaultDarkLookAndFeel());
    m_settings->save();
    return true;
}

#include "moc_config.cpp"
