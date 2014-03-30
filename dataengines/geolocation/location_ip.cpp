/*
 *   Copyright (C) 2009 Petri Damstén <damu@iki.fi>
 *                  - Original Implementation.
 *                 2009 Andrew Coles  <andrew.coles@yahoo.co.uk>
 *                  - Extension to iplocationtools engine.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "location_ip.h"
#include <QDebug>
#include <QUrl>
#include <KJob>
#include <KIO/Job>
#include <KIO/TransferJob>

class Ip::Private : public QObject {

public:
    QByteArray payload;

    void populateDataEngineData(Plasma::DataEngine::Data & outd)
    {
        QString country, countryCode, city, latitude, longitude, ip;
        const QList<QByteArray> &bl = payload.split('\n');
        payload.clear();
        foreach (const QByteArray &b, bl) {
            const QList<QByteArray> &t = b.split(':');
            if (t.count() > 1) {
                const QByteArray k = t[0];
                const QByteArray v = t[1];
                if (k == "Latitude") {
                    latitude = v;
                } else if (k == "Longitude") {
                    longitude = v;
                } else if (k == "Country") {
                    QStringList cc = QString(v).split('(');
                    if (cc.count() > 1) {
                        country = cc[0].trimmed();
                        countryCode = cc[1].replace(')', "");
                    }
                } else if (k == "City") {
                    city = v;
                } else if (k == "IP") {
                    ip = v;
                }
            }
        }
        // ordering of first three to preserve backwards compatibility
        outd["accuracy"] = 40000;
        outd["country"] = country;
        outd["country code"] = countryCode;
        outd["city"] = city;
        outd["latitude"] = latitude;
        outd["longitude"] = longitude;
        outd["ip"] = ip;
    }
};

Ip::Ip(QObject* parent, const QVariantList& args)
    : GeolocationProvider(parent, args), d(new Private())
{
    setUpdateTriggers(SourceEvent | NetworkConnected);
}

Ip::~Ip()
{
    delete d;
}

void Ip::update()
{
    d->payload.clear();
    KIO::TransferJob *datajob = KIO::get(QUrl("http://api.hostip.info/get_html.php?position=true"),
                                         KIO::NoReload, KIO::HideProgressInfo);

    if (datajob) {
        qDebug() << "Fetching http://api.hostip.info/get_html.php?position=true";
        connect(datajob, SIGNAL(data(KIO::Job*,QByteArray)), this,
                SLOT(readData(KIO::Job*,QByteArray)));
        connect(datajob, SIGNAL(result(KJob*)), this, SLOT(result(KJob*)));
    } else {
        qDebug() << "Could not create job";
    }
}

void Ip::readData(KIO::Job* job, const QByteArray& data)
{
    Q_UNUSED(job)

    if (data.isEmpty()) {
        return;
    }
    d->payload.append(data);
}

void Ip::result(KJob* job)
{
    Plasma::DataEngine::Data outd;

    if(job && !job->error()) {
        d->populateDataEngineData(outd);
    } else {
        qDebug() << "error" << job->errorString();
    }

    setData(outd);
}

K_EXPORT_PLASMA_GEOLOCATIONPROVIDER(ip, Ip)

#include "location_ip.moc"
