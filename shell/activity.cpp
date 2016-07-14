/*
 *   Copyright 2010 Chani Armitage <chani@kde.org>
 *   Copyright 2016 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
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

#include "shellcorona.h"
#include "kidenticongenerator.h"

#include <QAction>
#include <QDebug>
#include <QPixmap>
#include <QString>
#include <QSize>
#include <QFile>

#include <kactioncollection.h>
#include <kconfig.h>
#include <kwindowsystem.h>

#include <Plasma/Containment>
#include <Plasma/Corona>

#include <kactivities/controller.h>

#include "activity.h"

#include <memory>
// #include <mutex>

namespace {
    std::shared_ptr<KActivities::Controller> activitiesControllerInstance()
    {
        static std::weak_ptr<KActivities::Controller> s_instance;

        // TODO: If it turns out we need these in multiple threads,
        //       all hell will break loose (well, not really)
        // static std::mutex sharedSingleton;
        // std::lock_guard<std::mutex> sharedSingletonLock(sharedSingleton);

        auto result = s_instance.lock();

        if (s_instance.expired()) {
            result.reset(new KActivities::Controller());
            s_instance = result;
        }

        return result;
    }

}

Activity::Activity(const QString &id, Plasma::Corona *parent)
    : QObject(parent),
      m_info(id),
      m_plugin(QStringLiteral("org.kde.desktopcontainment")),//FIXME ask the corona
      m_activityController(activitiesControllerInstance())
{
    connect(&m_info, &KActivities::Info::stateChanged, this, &Activity::stateChanged);
    connect(&m_info, &KActivities::Info::started, this, &Activity::opened);
    connect(&m_info, &KActivities::Info::stopped, this, &Activity::closed);
    connect(&m_info, &KActivities::Info::removed, this, &Activity::removed);
    connect(&m_info, &KActivities::Info::removed, this, &Activity::cleanupActivity);
}

Activity::~Activity()
{
}

QString Activity::id() const
{
    return m_info.id();
}

QString Activity::name() const
{
    return m_info.name();
}

QPixmap Activity::pixmap(const QSize &size) const
{
    if (m_info.isValid() && !m_info.icon().isEmpty()) {
        return QIcon::fromTheme(m_info.icon()).pixmap(size);
    } else {
        return QIcon().pixmap(size);
    }
}

bool Activity::isCurrent() const
{
    return m_info.isCurrent();
}

KActivities::Info::State Activity::state() const
{
    return m_info.state();
}

void Activity::remove()
{
    m_activityController->removeActivity(m_info.id());
}

void Activity::cleanupActivity()
{
    const QString name = "activities/" + m_info.id();
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QChar('/')+name);
}

void Activity::activate()
{
    m_activityController->setCurrentActivity(m_info.id());
}

void Activity::setName(const QString &name)
{
    m_activityController->setActivityName(m_info.id(), name);
}

void Activity::setIcon(const QString &icon)
{
    m_activityController->setActivityIcon(m_info.id(), icon);
}

void Activity::close()
{
    m_activityController->stopActivity(m_info.id());
}

KConfigGroup Activity::config() const
{
    const QString name = "activities/" + m_info.id();
    KConfig external(name, KConfig::SimpleConfig, QStandardPaths::GenericDataLocation);

    //passing an empty string for the group name turns a kconfig into a kconfiggroup
    return external.group(QString());
}

void Activity::open()
{
    m_activityController->startActivity(m_info.id());
}

void Activity::setDefaultPlugin(const QString &plugin)
{
    m_plugin = plugin;
    //FIXME save&restore this setting
}

QString Activity::defaultPlugin() const
{
    return m_plugin;
}

const KActivities::Info * Activity::info() const
{
    return &m_info;
}



// vim: sw=4 sts=4 et tw=100
