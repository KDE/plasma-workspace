/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

// plasma
#include <Plasma/DataEngine>
#include <Plasma/Service>

#include <KService>
#include <KServiceGroup>

/**
 * Apps Data Engine
 *
 * FIXME
 * This engine provides information regarding tasks (windows that are currently open)
 * as well as startup tasks (windows that are about to open).
 * Each task and startup is represented by a unique source. Sources are added and removed
 * as windows are opened and closed. You cannot request a customized source.
 *
 * A service is also provided for each task. It exposes some operations that can be
 * performed on the windows (ex: maximize, minimize, activate).
 *
 * The data and operations are provided and handled by the taskmanager library.
 * It should be noted that only a subset of data and operations are exposed.
 */
class AppsEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    AppsEngine(QObject *parent, const QVariantList &args);
    ~AppsEngine() override;
    Plasma::Service *serviceForSource(const QString &name) override;

protected:
    virtual void init();

private:
    friend class AppSource;
    void addGroup(KServiceGroup::Ptr group);
    void addApp(KService::Ptr app);
};
