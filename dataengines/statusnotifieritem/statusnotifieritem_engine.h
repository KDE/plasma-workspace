/*
    SPDX-FileCopyrightText: 2009 Marco Martin <notmart@gmail.com>
    SPDX-FileCopyrightText: 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "statusnotifierwatcher_interface.h"
#include <Plasma5Support/DataEngine>
#include <Plasma5Support/Service>
#include <QDBusConnection>

// Define our plasma Runner
class StatusNotifierItemEngine : public Plasma5Support::DataEngine
{
    Q_OBJECT

public:
    // Basic Create/Destroy
    StatusNotifierItemEngine(QObject *parent, const QVariantList &args);
    ~StatusNotifierItemEngine() override;
    Plasma5Support::Service *serviceForSource(const QString &name) override;

protected:
    virtual void init();
    void newItem(const QString &service);

protected Q_SLOTS:
    void serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner);
    void registerWatcher(const QString &service);
    void unregisterWatcher(const QString &service);
    void serviceRegistered(const QString &service);
    void serviceUnregistered(const QString &service);

private:
    org::kde::StatusNotifierWatcher *m_statusNotifierWatcher;
    QString m_serviceName;
    static const int s_protocolVersion = 0;
};
