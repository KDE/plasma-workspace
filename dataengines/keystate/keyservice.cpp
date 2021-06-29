/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "keyservice.h"

#include <kmodifierkeyinfo.h>

KeyService::KeyService(QObject *parent, KModifierKeyInfo *keyInfo, Qt::Key key)
    : Plasma::Service(parent)
    , m_keyInfo(keyInfo)
    , m_key(key)
{
    setName(QStringLiteral("modifierkeystate"));
    setDestination(QStringLiteral("keys"));
}

Plasma::ServiceJob *KeyService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    if (operation == QLatin1String("Lock")) {
        return new LockKeyJob(this, parameters);
    } else if (operation == QLatin1String("Latch")) {
        return new LatchKeyJob(this, parameters);
    }

    return nullptr;
}

void KeyService::lock(bool lock)
{
    m_keyInfo->setKeyLocked(m_key, lock);
}

void KeyService::latch(bool lock)
{
    m_keyInfo->setKeyLatched(m_key, lock);
}

LockKeyJob::LockKeyJob(KeyService *service, const QMap<QString, QVariant> &parameters)
    : Plasma::ServiceJob(service->destination(), QStringLiteral("Lock"), parameters, service)
    , m_service(service)
{
}

void LockKeyJob::start()
{
    m_service->lock(parameters().value(QStringLiteral("Lock")).toBool());
    setResult(true);
}

LatchKeyJob::LatchKeyJob(KeyService *service, const QMap<QString, QVariant> &parameters)
    : Plasma::ServiceJob(service->destination(), QStringLiteral("Lock"), parameters, service)
    , m_service(service)
{
}

void LatchKeyJob::start()
{
    m_service->latch(parameters().value(QStringLiteral("Lock")).toBool());
    setResult(true);
}

// vim: sw=4 sts=4 et tw=100
