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

AppSource::AppSource(KServiceGroup::Ptr group, QObject *parent) :
    Plasma::DataContainer(parent),
    m_group(group),
    m_app(),
    m_isApp(false)
{
    setObjectName(m_group->entryPath());
    setData("isApp", false);
    updateGroup();
}

AppSource::AppSource(KService::Ptr app, QObject *parent) :
    Plasma::DataContainer(parent),
    m_group(),
    m_app(app),
    m_isApp(true)
{
    setObjectName(m_app->storageId());
    setData("isApp", true);
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
    setData("iconName", m_group->icon());
    setData("name", m_group->caption());
    setData("comment", m_group->comment());
    setData("display", !m_group->noDisplay());

    QStringList entries;
    foreach (KSycocaEntry::Ptr p, m_group->entries(true, false, true)) {
        if (p->isType(KST_KService)) {
            const KService::Ptr service = p;
            entries << service->storageId();
        } else if (p->isType(KST_KServiceGroup)) {
            const KServiceGroup::Ptr service = p;
            entries << service->entryPath();
        } else if (p->isType(KST_KServiceSeparator)) {
            entries << "---";
        } else {
            qDebug() << "unexpected object in entry list";
        }
    }
    setData("entries", entries);

    checkForUpdate();
}

void AppSource::updateApp()
{
    setData("iconName", m_app->icon());
    setData("name", m_app->name());
    setData("genericName", m_app->genericName());
    setData("menuId", m_app->menuId());
    setData("entryPath", m_app->entryPath());
    setData("comment", m_app->comment());
    setData("keywords", m_app->keywords());
    setData("categories", m_app->categories());
    setData("display", !m_app->noDisplay());
    checkForUpdate();
}

#include "appsource.moc"
