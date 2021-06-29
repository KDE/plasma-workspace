/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "appsengine.h"
#include "appsource.h"

#include <KSycoca>

AppsEngine::AppsEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
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
    connect(KSycoca::self(), QOverload<>::of(&KSycoca::databaseChanged), this, [this]() {
        removeAllSources();
        addGroup(KServiceGroup::root());
    });
}

Plasma::Service *AppsEngine::serviceForSource(const QString &name)
{
    AppSource *source = dynamic_cast<AppSource *>(containerForSource(name));
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
    // TODO listen for changes
    addSource(appSource);
    // do children
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
    // TODO listen for changes
    addSource(appSource);
}

K_PLUGIN_CLASS_WITH_JSON(AppsEngine, "plasma-dataengine-apps.json")

#include "appsengine.moc"
