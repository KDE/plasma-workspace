/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <Plasma/Service>
#include <Plasma/ServiceJob>

class KModifierKeyInfo;

class KeyService : public Plasma::Service
{
    Q_OBJECT

public:
    KeyService(QObject *parent, KModifierKeyInfo *keyInfo, Qt::Key key);
    void lock(bool lock);
    void latch(bool lock);

protected:
    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

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
