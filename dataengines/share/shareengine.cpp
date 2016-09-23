/***************************************************************************
 *   Copyright 2010 Artur Duque de Souza <asouza@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include <QDebug>
#include <KSycoca>
#include <KServiceTypeTrader>

#include <Plasma/DataContainer>

#include "shareengine.h"
#include "shareservice.h"


ShareEngine::ShareEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
    Q_UNUSED(args);
    init();
}

void ShareEngine::init()
{
    connect(KSycoca::self(), SIGNAL(databaseChanged(QStringList)),
            this, SLOT(updatePlugins(QStringList)));
    updatePlugins(QStringList() << QStringLiteral("services"));
}

void ShareEngine::updatePlugins(const QStringList &changes)
{
    if (!changes.contains(QStringLiteral("services"))) {
        return;
    }

    removeAllSources();

    KService::List services = KServiceTypeTrader::self()->query(QStringLiteral("Plasma/ShareProvider"));
    QMultiMap<int, KService::Ptr> sortedServices;
    foreach (KService::Ptr service, services) {
        sortedServices.insert(service->property(QStringLiteral("X-KDE-Priority")).toInt(), service);
    }

    QMapIterator<int, KService::Ptr> it(sortedServices);
    it.toBack();
    QHash<QString, QStringList> mimetypes;
    while (it.hasPrevious()) {
        it.previous();
        KService::Ptr service = it.value();
        const QString pluginName =
            service->property(QStringLiteral("X-KDE-PluginInfo-Name"), QVariant::String).toString();

        const QStringList pluginMimeTypes =
            service->property(QStringLiteral("X-KDE-PlasmaShareProvider-MimeType"), QVariant::StringList).toStringList();

        if (pluginName.isEmpty() || pluginMimeTypes.isEmpty()) {
            continue;
        }

        // create the list of providers
        Plasma::DataEngine::Data data;
        data.insert(QStringLiteral("Name"), service->name());
        data.insert(QStringLiteral("Service Id"), service->storageId());
        data.insert(QStringLiteral("Mimetypes"), pluginMimeTypes);
        setData(pluginName, data);

        // create the list of providers by type
        foreach (const QString &pluginMimeType, pluginMimeTypes) {
            mimetypes[pluginMimeType].append(pluginName);
        }
    }


    QHashIterator<QString, QStringList> it2(mimetypes);
    while (it2.hasNext()) {
        it2.next();
        setData(QStringLiteral("Mimetypes"), it2.key(), it2.value());
    }
}

Plasma::Service *ShareEngine::serviceForSource(const QString &source)
{
    Plasma::DataContainer *data = containerForSource(source);

    if (!data) {
        return Plasma::DataEngine::serviceForSource(source);
    }

    if (source.compare(QLatin1String("mimetype"), Qt::CaseInsensitive) == 0) {
        return Plasma::DataEngine::serviceForSource(source);
    }

    const QString id = data->data().value(QStringLiteral("Service Id")).toString();
    if (id.isEmpty()) {
        return Plasma::DataEngine::serviceForSource(source);
    }

    ShareService *service = new ShareService(this);
    service->setDestination(id);
    return service;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(share, ShareEngine, "plasma-dataengine-share.json")

#include "shareengine.moc"

