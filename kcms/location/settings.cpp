/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "settings.h"

#include <KAuth/Action>
#include <KAuth/ExecuteJob>

#include <QDebug>

ManualLocationItem::ManualLocationItem(ManualLocation &location)
    : KConfigSkeletonItem(QString(), QStringLiteral("ManualLocation"))
    , m_reference(location)
{
    setGetDefaultImpl([this]() {
        return QVariant::fromValue(m_default);
    });
    setIsDefaultImpl([this]() {
        return m_reference == m_default;
    });
    setIsSaveNeededImpl([this]() {
        return m_reference != m_loaded;
    });
}

void ManualLocationItem::readConfig(KConfig *config)
{
    Q_UNUSED(config)
    KAuth::Action query(QStringLiteral("org.kde.kcm.location.getlocation"));
    query.setHelperId(QStringLiteral("org.kde.kcm.location"));

    KAuth::ExecuteJob *job = query.execute();
    if (job->exec()) {
        const QVariantMap data = job->data();

        m_reference.present = data[QStringLiteral("present")].toBool();
        m_reference.coordinate = QGeoCoordinate(data[QStringLiteral("latitude")].toDouble(),
                                                data[QStringLiteral("longitude")].toDouble(),
                                                data[QStringLiteral("altitude")].toDouble());
    } else {
        qWarning() << "Failed to query static geolocation source:" << job->errorString();
    }

    m_loaded = m_reference;
}

void ManualLocationItem::writeConfig(KConfig *config)
{
    Q_UNUSED(config)
    if (m_reference != m_loaded) {
        if (!m_reference.present) {
            KAuth::Action action(QStringLiteral("org.kde.kcm.location.unsetlocation"));
            action.setHelperId(QStringLiteral("org.kde.kcm.location"));

            KAuth::ExecuteJob *job = action.execute();
            if (!job->exec()) {
                qWarning() << "Failed to turn on automatic location detection:" << job->errorString();
            }
        } else {
            KAuth::Action action(QStringLiteral("org.kde.kcm.location.setlocation"));
            action.setHelperId(QStringLiteral("org.kde.kcm.location"));
            action.setArguments({
                {QStringLiteral("latitude"), QVariant::fromValue(m_reference.coordinate.latitude())},
                {QStringLiteral("longitude"), QVariant::fromValue(m_reference.coordinate.longitude())},
                {QStringLiteral("altitude"), QVariant::fromValue(m_reference.coordinate.altitude())},
            });

            KAuth::ExecuteJob *job = action.execute();
            if (!job->exec()) {
                qWarning() << "Failed to set manual location:" << job->errorString();
            }
        }

        m_loaded = m_reference;
    }
}

void ManualLocationItem::readDefault(KConfig *config)
{
    Q_UNUSED(config)
    m_default = ManualLocation{};
}

void ManualLocationItem::setDefault()
{
    m_reference = m_default;
}

void ManualLocationItem::swapDefault()
{
    std::swap(m_reference, m_default);
}

bool ManualLocationItem::isEqual(const QVariant &p) const
{
    if (!p.canConvert<ManualLocation>()) {
        return false;
    }
    return p.value<ManualLocation>() == m_reference;
}

QVariant ManualLocationItem::property() const
{
    return QVariant::fromValue(m_reference);
}

void ManualLocationItem::setProperty(const QVariant &p)
{
    m_reference = p.value<ManualLocation>();
}

LocationSettings::LocationSettings(QObject *parent)
    : KConfigSkeleton(QString(), parent)
{
    addItem(new KConfigCompilerSignallingItem(new ManualLocationItem(m_manualLocation),
                                              this,
                                              static_cast<KConfigCompilerSignallingItem::NotifyFunction>(&LocationSettings::itemChanged),
                                              0));
}

ManualLocation LocationSettings::manualLocation() const
{
    return m_manualLocation;
}

void LocationSettings::setManualLocation(const ManualLocation &location)
{
    if (m_manualLocation != location) {
        m_manualLocation = location;
        Q_EMIT manualLocationChanged();
    }
}

void LocationSettings::itemChanged(quint64 flags)
{
    Q_UNUSED(flags)
    Q_EMIT manualLocationChanged();
}

LocationData::LocationData(QObject *parent)
    : KCModuleData(parent)
    , m_settings(new LocationSettings(this))
{
    autoRegisterSkeletons();
}

LocationSettings *LocationData::settings() const
{
    return m_settings;
}

#include "moc_settings.cpp"
