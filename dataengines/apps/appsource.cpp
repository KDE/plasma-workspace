/*
 * Copyright 2009 Chani Armitage <chani@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "appsource.h"
#include "appsengine.h"
#include "appservice.h"

#include <QDebug>

AppSource::AppSource(KServiceGroup::Ptr group, QObject *parent)
    : Plasma::DataContainer(parent)
    , m_group(group)
    , m_app()
    , m_isApp(false)
{
    setObjectName(m_group->entryPath());
    setData(QStringLiteral("isApp"), false);
    updateGroup();
}

AppSource::AppSource(KService::Ptr app, QObject *parent)
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
