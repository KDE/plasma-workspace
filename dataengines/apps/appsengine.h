/*
 * Copyright 2009 Chani Armitage <chani@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef APPSENGINE_H
#define APPSENGINE_H

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

    private Q_SLOTS:
        void sycocaChanged(const QStringList &changes);

    private:
        friend class AppSource;
        void addGroup(KServiceGroup::Ptr group);
        void addApp(KService::Ptr app);
};

#endif // TASKSENGINE_H
