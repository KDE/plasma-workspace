/*
 * Copyright 2009 Aaron Seigo aseigo@kde.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#ifndef KEYSERVICE_H
#define KEYSERVICE_H

#include <Plasma/Service>
#include <Plasma/ServiceJob>

class KModifierKeyInfo;

class KeyService : public Plasma::Service
{
    Q_OBJECT

public:
    KeyService(QObject* parent, KModifierKeyInfo *keyInfo, Qt::Key key);
    void lock(bool lock);
    void latch(bool lock);

protected:
    Plasma::ServiceJob* createJob(const QString& operation, QMap<QString,QVariant>& parameters) override;

private:
    KModifierKeyInfo *m_keyInfo;
    Qt::Key m_key;
};

class LockKeyJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    LockKeyJob(KeyService *service, const QMap<QString, QVariant> &parameters);
    void start() override;

private:
    KeyService *m_service;
};

class LatchKeyJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    LatchKeyJob(KeyService *service, const QMap<QString, QVariant> &parameters);
    void start() override;

private:
    KeyService *m_service;
};

#endif // KEYSERVICE_H
