/*
 *   Copyright 2010 Chani Armitage <chani@kde.org>
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
#include <kactivities/consumer.h>

#include "activity.h"

Activity::Activity(const QString &id, Plasma::Corona *parent)
    : QObject(parent),
      m_id(id),
      m_plugin("org.kde.desktopcontainment"),//FIXME ask the corona
      m_info(new KActivities::Info(id, this)),
      m_activityConsumer(new KActivities::Consumer(this)),
      m_current(false)
{
    m_name = m_info->name();
    m_icon = m_info->icon();

    connect(m_info, SIGNAL(infoChanged()), this, SLOT(activityChanged()));
    connect(m_info, SIGNAL(stateChanged(KActivities::Info::State)), this, SIGNAL(stateChanged()));
    connect(m_info, SIGNAL(started()), this, SIGNAL(opened()));
    connect(m_info, SIGNAL(stopped()), this, SIGNAL(closed()));
    connect(m_info, SIGNAL(removed()), this, SIGNAL(removed()));
    connect(m_info, SIGNAL(removed()), this, SLOT(cleanupActivity()));

    connect(m_activityConsumer, SIGNAL(currentActivityChanged(QString)), this, SLOT(checkIfCurrent()));
    checkIfCurrent();
}

Activity::~Activity()
{
}

void Activity::activityChanged()
{
    setName(m_info->name());
    setIcon(m_info->icon());
}

QString Activity::id()
{
    return m_id;
}

QString Activity::name()
{
    return m_name;
}

QPixmap Activity::pixmap(const QSize &size)
{
    if (m_info->isValid() && !m_info->icon().isEmpty()) {
        return QIcon::fromTheme(m_info->icon()).pixmap(size);
    } else {
        return KIdenticonGenerator::self()->generatePixmap(size.width(), m_id);
    }
}

bool Activity::isCurrent()
{
    return m_current;
    //TODO maybe plasmaapp should cache the current activity to reduce dbus calls?
}

void Activity::checkIfCurrent()
{
    const bool current = m_id == m_activityConsumer->currentActivity();
    if (current != m_current) {
        m_current = current;
        emit currentStatusChanged();
    }
}

KActivities::Info::State Activity::state()
{
    return m_info->state();
}

void Activity::remove()
{
    KActivities::Controller().removeActivity(m_id);
}

void Activity::cleanupActivity()
{
    const QString name = "activities/" + m_id;
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QChar('/')+name);
}

void Activity::activate()
{
    KActivities::Controller().setCurrentActivity(m_id);
}

void Activity::setName(const QString &name)
{
    m_name = name;
}

void Activity::setIcon(const QString &icon)
{
    m_icon = icon;
}

void Activity::close()
{
    KActivities::Controller().stopActivity(m_id);
}

KConfigGroup Activity::config() const
{
    const QString name = "activities/" + m_id;
    KConfig external(name, KConfig::SimpleConfig, QStandardPaths::GenericDataLocation);

    //passing an empty string for the group name turns a kconfig into a kconfiggroup
    return external.group(QString());
}

void Activity::open()
{
    KActivities::Controller().startActivity(m_id);
}

void Activity::setDefaultPlugin(const QString &plugin)
{
    m_plugin = plugin;
    //FIXME save&restore this setting
}

const KActivities::Info * Activity::info() const
{
    return m_info;
}

#include "activity.moc"

// vim: sw=4 sts=4 et tw=100
