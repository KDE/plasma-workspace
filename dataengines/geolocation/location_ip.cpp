/*
    SPDX-FileCopyrightText: 2009 Petri Damstén <damu@iki.fi>

    Original Implementation:
    SPDX-FileCopyrightText: 2009 Andrew Coles <andrew.coles@yahoo.co.uk>

    Extension to iplocationtools engine:
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "location_ip.h"
#include "geolocdebug.h"
#include <KSharedConfig>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/WirelessDevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

class Ip::Private : public QObject
{
    Q_OBJECT
public:
    Private(Ip *q)
        : q(q)
    {
        m_nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
        m_nam.setStrictTransportSecurityEnabled(true);
        m_nam.enableStrictTransportSecurityStore(true,
                                                 QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/plasmashell/hsts/"));
    }

    void readGeoLocation(QNetworkReply *reply)
    {
        m_geoLocationResolved = true;
        if (reply->error()) {
            qCCritical(DATAENGINE_GEOLOCATION) << "error: " << reply->errorString();
            checkUpdateData();
            return;
        }
        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();

        auto accuracyIt = json.find(QStringLiteral("accuracy"));
        if (accuracyIt != json.end()) {
            m_data[QStringLiteral("accuracy")] = (*accuracyIt).toDouble();
        } else {
            m_data[QStringLiteral("accuracy")] = 40000;
        }

        auto locationIt = json.find(QStringLiteral("location"));
        if (locationIt != json.end()) {
            QJsonObject location = (*locationIt).toObject();
            m_data[QStringLiteral("latitude")] = location.value(QStringLiteral("lat")).toDouble();
            m_data[QStringLiteral("longitude")] = location.value(QStringLiteral("lng")).toDouble();
        }
        checkUpdateData();
    }

    void clear()
    {
        m_countryResolved = false;
        m_geoLocationResolved = false;
        m_data.clear();
    }

    void readCountry(QNetworkReply *reply)
    {
        m_countryResolved = true;
        if (reply->error()) {
            qCCritical(DATAENGINE_GEOLOCATION) << "error: " << reply->errorString();
            checkUpdateData();
            return;
        }

        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();

        m_data[QStringLiteral("country")] = json.value(QStringLiteral("country_name")).toString();
        m_data[QStringLiteral("country code")] = json.value(QStringLiteral("country_code")).toString();

        checkUpdateData();
    }

    void checkUpdateData()
    {
        if (!m_countryResolved || !m_geoLocationResolved) {
            return;
        }
        q->setData(m_data);
    }

    Ip *q;
    bool m_countryResolved = false;
    bool m_geoLocationResolved = false;
    Plasma5Support::DataEngine::Data m_data;
    QNetworkAccessManager m_nam;
};

Ip::Ip(QObject *parent)
    : GeolocationProvider(parent)
    , d(new Private(this))
{
    setUpdateTriggers(SourceEvent | NetworkConnected);
}

Ip::~Ip()
{
    delete d;
}

static QJsonArray accessPoints()
{
    QJsonArray wifiAccessPoints;
    const KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("org.kde.plasma.geolocation.ip"));
    if (!NetworkManager::isWirelessEnabled() || !config.readEntry("Wifi", false)) {
        return wifiAccessPoints;
    }
    for (const auto &device : NetworkManager::networkInterfaces()) {
        QSharedPointer<NetworkManager::WirelessDevice> wifi = qSharedPointerDynamicCast<NetworkManager::WirelessDevice>(device);
        if (!wifi) {
            continue;
        }
        for (const auto &network : wifi->networks()) {
            const QString &ssid = network->ssid();
            if (ssid.isEmpty() || ssid.endsWith(QLatin1String("_nomap"))) {
                // skip hidden SSID and networks with "_nomap"
                continue;
            }
            for (const auto &accessPoint : network->accessPoints()) {
                wifiAccessPoints.append(QJsonObject{{QStringLiteral("macAddress"), accessPoint->hardwareAddress()}});
            }
        }
    }
    return wifiAccessPoints;
}

void Ip::update()
{
    d->clear();
    if (!NetworkManager::isNetworkingEnabled()) {
        setData(Plasma5Support::DataEngine::Data());
        return;
    }
    const QJsonArray wifiAccessPoints = accessPoints();
    QJsonObject request;
    if (wifiAccessPoints.count() >= 2) {
        request.insert(QStringLiteral("wifiAccessPoints"), wifiAccessPoints);
    }
    const QByteArray postData = QJsonDocument(request).toJson(QJsonDocument::Compact);
    const QString apiKey = QStringLiteral("60e8eae6-3988-4ada-ad48-2cfddddf216b");

    qCDebug(DATAENGINE_GEOLOCATION) << "Fetching https://location.services.mozilla.com/v1/geolocate";
    QNetworkRequest locationRequest(QUrl(QStringLiteral("https://location.services.mozilla.com/v1/geolocate?key=%1").arg(apiKey)));
    locationRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *locationReply = d->m_nam.post(locationRequest, postData);

    connect(locationReply, &QNetworkReply::finished, this, [this, locationReply] {
        locationReply->deleteLater();
        d->readGeoLocation(locationReply);
    });

    qCDebug(DATAENGINE_GEOLOCATION) << "Fetching https://location.services.mozilla.com/v1/country";
    QNetworkRequest countryRequest(QUrl(QStringLiteral("https://location.services.mozilla.com/v1/country?key=%1").arg(apiKey)));
    countryRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *countryReply = d->m_nam.post(countryRequest, postData);

    connect(countryReply, &QNetworkReply::finished, this, [this, countryReply] {
        countryReply->deleteLater();
        d->readCountry(countryReply);
    });
}

K_PLUGIN_CLASS_WITH_JSON(Ip, "plasma-geolocation-ip.json")

#include "location_ip.moc"
