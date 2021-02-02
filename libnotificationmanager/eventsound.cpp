/*
 * Copyright 2021 Kai Uwe Broulik <kde@broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "eventsound.h"

#include <QFile>
#include <QScopeGuard>
#include <QString>

#include "debug.h"
#include "notifications.h"

#include <canberra.h>

using namespace NotificationManager;

class NotificationManager::EventSoundPrivate
{
public:
    QString schemeName;
    QString eventId;
    QString path;

    QString applicationName;
    QString desktopEntry;
    QString applicationIconName;

    ca_context *context = nullptr;

    bool init();
};

bool EventSoundPrivate::init()
{
    if (context) {
        // TODO probably always change_props in case the properties were modified before play()?
        return true;
    }

    int ret = ca_context_create(&context);
    if (ret != CA_SUCCESS) {
        qCWarning(NOTIFICATIONMANAGER) << "Failed to initialize canberra context for audio notification:" << ca_strerror(ret);
        context = nullptr;
        return false;
    }

    ret = ca_context_change_props(context,
        CA_PROP_APPLICATION_NAME, qUtf8Printable(applicationName),
        CA_PROP_APPLICATION_ID, qUtf8Printable(desktopEntry),
        CA_PROP_APPLICATION_ICON_NAME, qUtf8Printable(applicationIconName),
        // TODO just QLocale().bcp47Name or pass it in from QML?
        //CA_PROP_APPLICATION_LANGUAGE
        nullptr);
    if (ret != CA_SUCCESS) {
        qCWarning(NOTIFICATIONMANAGER) << "Failed to set application properties on canberra context for audio notification:" << ca_strerror(ret);
        // TODO return false?
    }

    return true;
}

EventSound::EventSound(QObject *parent)
    : QObject(parent)
    , d(new EventSoundPrivate())
{
    //qRegisterMetaType<uint32_t>("uint32_t");
}

EventSound::EventSound(const QString &schemeName, const QModelIndex &idx, QObject *parent)
    : EventSound(parent)
{
    d->schemeName = schemeName;
    d->eventId = idx.data(Notifications::SoundNameRole).toString();
    d->path = idx.data(Notifications::SoundFileRole).toString();

    // TODO check model?
    d->applicationName = idx.data(Notifications::ApplicationNameRole).toString();
    d->applicationIconName = idx.data(Notifications::ApplicationIconNameRole).toString();
}

EventSound::~EventSound()
{
    if (d->context) {
        // TODO What if QML gc kicks in whilst still playing?
        ca_context_destroy(d->context);
    }
}

void EventSound::play()
{
    Q_ASSERT(!d->eventId.isEmpty() || !d->path.isEmpty());

    if (!d->init()) {
        return;
    }

    ca_proplist *props = nullptr;
    ca_proplist_create(&props);
    auto cleanup = qScopeGuard([&props] {
        ca_proplist_destroy(props);
    });

    qCDebug(NOTIFICATIONMANAGER) << "Play event sound for event" << d->eventId << "or path" << d->path
                                 << "on scheme" << d->schemeName << "for application" << d->desktopEntry << d->applicationName;

    ca_proplist_sets(props, CA_PROP_EVENT_ID, qPrintable(d->eventId));
    ca_proplist_sets(props, CA_PROP_MEDIA_FILENAME, QFile::encodeName(d->path).constData());
    // We'll also want this cached for a time. volatile makes sure the cache is
    // dropped after some time or when the cache is under pressure.
    ca_proplist_sets(props, CA_PROP_CANBERRA_CACHE_CONTROL, "volatile");

    ca_proplist_sets(props, CA_PROP_CANBERRA_XDG_THEME_NAME, qPrintable(d->schemeName));
    // TODO CA_PROP_CANBERRA_XDG_THEME_OUTPUT_PROFILE?

    // we probably don't need the finished callback
    int ret = ca_context_play_full(d->context, 0, props, nullptr, nullptr);

    if (ret != CA_SUCCESS) {
        qCWarning(NOTIFICATIONMANAGER) << "Failed to play sound with canberra:" << ca_strerror(ret);
        return;
    }

}
