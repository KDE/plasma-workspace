/*
 * Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * This library is free software; you can redistribute it and/or
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "keyservice.h"

#include <kmodifierkeyinfo.h>

KeyService::KeyService(QObject* parent, KModifierKeyInfo *keyInfo, Qt::Key key)
    : Plasma::Service(parent),
      m_keyInfo(keyInfo),
      m_key(key)
{
    setName("modifierkeystate");
    setDestination("keys");
}

Plasma::ServiceJob* KeyService::createJob(const QString& operation, QMap<QString,QVariant>& parameters)
{
    if (operation == "Lock") {
        return new LockKeyJob(this, parameters);
    } else if (operation == "Latch") {
        return new LatchKeyJob(this, parameters);
    }

    return 0;
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
    : Plasma::ServiceJob(service->destination(), "Lock", parameters, service),
      m_service(service)
{
}

void LockKeyJob::start()
{
    m_service->lock(parameters().value("Lock").toBool());
    setResult(true);
}

LatchKeyJob::LatchKeyJob(KeyService *service, const QMap<QString, QVariant> &parameters)
    : Plasma::ServiceJob(service->destination(), "Lock", parameters, service),
      m_service(service)
{
}

void LatchKeyJob::start()
{
    m_service->latch(parameters().value("Lock").toBool());
    setResult(true);
}

#include "keyservice.moc"

// vim: sw=4 sts=4 et tw=100
