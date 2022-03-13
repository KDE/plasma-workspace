/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "appsource.h"
#include "appsengine.h"
#include "appservice.h"

#include <QDebug>

AppSource::AppSource(const KServiceGroup::Ptr &group, QObject *parent)
    : Plasma::DataContainer(parent)
    , m_group(group)
    , m_app()
    , m_isApp(false)
{
    setObjectName(m_group->entryPath());
    setData(QStringLiteral("isApp"), false);
    updateGroup();
}

AppSource::AppSource(const KService::Ptr &app, QObject *parent)
    : Plasma::DataContainer(parent)
    , m_group()
    , m_app(app)
    , m_isApp(true)
{
    setObjectName(m_app->storageId());
    setData(QStringLiteral("isApp"), true);
    updateApp();
}

AppSource::~AppSource()
{
}

Plasma::Service *AppSource::createService()
{
    return new AppService(this);
}

KService::Ptr AppSource::getApp()
{
    return m_app;
}

bool AppSource::isApp() const
{
    return m_isApp;
}

void AppSource::updateGroup()
{
    setData(QStringLiteral("iconName"), m_group->icon());
    setData(QStringLiteral("name"), m_group->caption());
    setData(QStringLiteral("comment"), m_group->comment());
    setData(QStringLiteral("display"), !m_group->noDisplay());

    QStringList entries;
    const auto groupEntries = m_group->entries(true, false, true);
    for (const KSycocaEntry::Ptr &p : groupEntries) {
        if (p->isType(KST_KService)) {
            const KService::Ptr service(static_cast<KService *>(p.data()));
            entries << service->storageId();
        } else if (p->isType(KST_KServiceGroup)) {
            const KServiceGroup::Ptr serviceGroup(static_cast<KServiceGroup *>(p.data()));
            entries << serviceGroup->entryPath();
        } else if (p->isType(KST_KServiceSeparator)) {
            entries << QStringLiteral("---");
        } else {
            qDebug() << "unexpected object in entry list";
        }
    }
    setData(QStringLiteral("entries"), entries);

    checkForUpdate();
}

void AppSource::updateApp()
{
    setData(QStringLiteral("iconName"), m_app->icon());
    setData(QStringLiteral("name"), m_app->name());
    setData(QStringLiteral("genericName"), m_app->genericName());
    setData(QStringLiteral("menuId"), m_app->menuId());
    setData(QStringLiteral("entryPath"), m_app->entryPath());
    setData(QStringLiteral("comment"), m_app->comment());
    setData(QStringLiteral("keywords"), m_app->keywords());
    setData(QStringLiteral("categories"), m_app->categories());
    setData(QStringLiteral("display"), !m_app->noDisplay());
    checkForUpdate();
}
