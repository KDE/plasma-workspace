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

#include "appsengine.h"
#include "appsource.h"

#include <KSycoca>

AppsEngine::AppsEngine(QObject *parent, const QVariantList &args) :
    Plasma::DataEngine(parent, args)
{
    Q_UNUSED(args);
    init();
}

AppsEngine::~AppsEngine()
{
}

void AppsEngine::init()
{
    addGroup(KServiceGroup::root());
    connect(KSycoca::self(), QOverload<const QStringList &>::of(&KSycoca::databaseChanged), this, &AppsEngine::sycocaChanged);
}

void AppsEngine::sycocaChanged(const QStringList &changes)
{
    if (changes.contains(QLatin1String("apps")) || changes.contains(QLatin1String("xdgdata-apps"))) {
        removeAllSources();
        addGroup(KServiceGroup::root());
    }
}

Plasma::Service *AppsEngine::serviceForSource(const QString &name)
{
    AppSource *source = dynamic_cast<AppSource*>(containerForSource(name));
    // if source does not exist, return null service
    if (!source) {
        return Plasma::DataEngine::serviceForSource(name);
    }

    // if source represents a group or something, return null service
    if (!source->isApp()) {
        return Plasma::DataEngine::serviceForSource(name);
    }
    // if source represent a proper app, return real service
    Plasma::Service *service = source->createService();
    service->setParent(this);
    return service;
}

void AppsEngine::addGroup(KServiceGroup::Ptr group)
{
    if (!(group && group->isValid())) {
        return;
    }
    AppSource *appSource = new AppSource(group, this);
    //TODO listen for changes
    addSource(appSource);
    //do children
    foreach (const KServiceGroup::Ptr &subGroup, group->groupEntries(KServiceGroup::NoOptions)) {
        addGroup(subGroup);
    }
    foreach (const KService::Ptr &app, group->serviceEntries(KServiceGroup::NoOptions)) {
        addApp(app);
    }
}

void AppsEngine::addApp(KService::Ptr app)
{
    AppSource *appSource = new AppSource(app, this);
    //TODO listen for changes
    addSource(appSource);
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(apps, AppsEngine, "plasma-dataengine-apps.json")

#include "appsengine.moc"
