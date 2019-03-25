/*
   This file is part of the KDE libraries
   Copyright (c) 2005-2010 David Jarvie <djarvie@kde.org>
   Copyright (c) 2005 S.R.Haque <srhaque@iee.org>
   Copyright (c) 2013 Martin Klapetek <mklapetek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "ktimezoned.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QTimeZone>
#include <QFileInfo>
#include <QDebug>

#include <KConfigGroup>
#include <KConfig>
#include <KDirWatch>
#include <KPluginFactory>
#include <KPluginLoader>

K_PLUGIN_CLASS_WITH_JSON(KTimeZoned, "ktimezoned.json")

const char LOCAL_ZONE[] = "LocalZone";     // name of local time zone
const char ZONEINFO_DIR[]   = "ZoneinfoDir";   // path to zoneinfo/ directory
const char ZONE_TAB[]       = "Zonetab";       // path & name of zone.tab

KTimeZoned::KTimeZoned(QObject* parent, const QList<QVariant>& l)
    : KTimeZonedBase(parent, l)
{
    init(false);
}

KTimeZoned::~KTimeZoned()
{
    delete m_dirWatch;
    m_dirWatch = nullptr;
    delete m_zoneTabWatch;
    m_zoneTabWatch = nullptr;
}

void KTimeZoned::init(bool restart)
{
    if (restart) {
        delete m_dirWatch;
        m_dirWatch = nullptr;
        delete m_zoneTabWatch;
        m_zoneTabWatch = nullptr;
        m_localZone = QString();
        m_zoneinfoDir = QString();
        m_zoneTab = QString();
    }

    KConfig config(QStringLiteral("ktimezonedrc"));
    if (restart)
        config.reparseConfiguration();

    KConfigGroup group(&config, "TimeZones");
    m_localZone = group.readEntry(LOCAL_ZONE);
    m_zoneinfoDir = group.readEntry(ZONEINFO_DIR);
    m_zoneTab = group.readEntry(ZONE_TAB);

    updateLocalZone();

    if (!m_dirWatch) {
        m_dirWatch = new KDirWatch(this);
        m_dirWatch->addFile(QStringLiteral("/etc/timezone"));
        m_dirWatch->addFile(QStringLiteral("/etc/localtime"));

        connect(m_dirWatch, &KDirWatch::dirty, this, &KTimeZoned::updateLocalZone);
        connect(m_dirWatch, &KDirWatch::deleted, this, &KTimeZoned::updateLocalZone);
        connect(m_dirWatch, &KDirWatch::created, this, &KTimeZoned::updateLocalZone);
    }

    if (!m_zoneTabWatch && findZoneTab(m_zoneTab)) {
        // cache the values so we don't look it up on next startup
        KConfig config(QStringLiteral("ktimezonedrc"));
        KConfigGroup group(&config, "TimeZones");
        group.writeEntry(ZONEINFO_DIR, m_zoneinfoDir);
        group.writeEntry(ZONE_TAB, m_zoneTab);
        group.sync();

        m_zoneTabWatch = new KDirWatch(this);
        m_zoneTabWatch->addDir(m_zoneinfoDir, KDirWatch::WatchFiles);

        connect(m_dirWatch, &KDirWatch::dirty, this, &KTimeZoned::updateLocalZone);
        connect(m_dirWatch, &KDirWatch::created, this, &KTimeZoned::updateLocalZone);
        connect(m_dirWatch, &KDirWatch::deleted, this, &KTimeZoned::updateLocalZone);
    }
}

// Check if the local zone has been updated, and if so, write the new
// zone to the config file and notify interested parties.
void KTimeZoned::updateLocalZone()
{
    QString systemTimeZone = QTimeZone::systemTimeZoneId();

    if (m_localZone != systemTimeZone) {
        qDebug() << "System timezone has been changed, new timezone is" << systemTimeZone;

        KConfig config(QStringLiteral("ktimezonedrc"));
        KConfigGroup group(&config, "TimeZones");
        m_localZone = systemTimeZone;
        group.writeEntry(LOCAL_ZONE, m_localZone);
        group.sync();

        QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/Daemon"), QStringLiteral("org.kde.KTimeZoned"), QStringLiteral("timeZoneChanged"));
        QDBusConnection::sessionBus().send(message);
    }
}

/*
 * Find the location of the zoneinfo files and store in m_zoneinfoDir.
 */
bool KTimeZoned::findZoneTab(const QString &pathFromConfig)
{
    // First try the cached path
    if (QFileInfo::exists(pathFromConfig)) {
        return true;
    }

    const QString ZONE_TAB_FILE = QStringLiteral("/zone.tab");
    const QString ZONE_INFO_DIR = QStringLiteral("/usr/share/zoneinfo");

    // Find and open zone.tab - it's all easy except knowing where to look.
    // Try the LSB location first.
    QDir dir;
    QString zoneinfoDir = ZONE_INFO_DIR;
    QString zoneTab = ZONE_INFO_DIR + ZONE_TAB_FILE;
    // make a note if the dir exists; whether it contains zone.tab or not
    if (dir.exists(zoneinfoDir) && QFileInfo::exists(zoneTab)) {
        m_zoneinfoDir = zoneinfoDir;
        m_zoneTab = zoneTab;
        return true;
    }

    zoneinfoDir = QStringLiteral("/usr/lib/zoneinfo");
    zoneTab = zoneinfoDir + ZONE_TAB_FILE;
    if (dir.exists(zoneinfoDir) && QFileInfo::exists(zoneTab)) {
        m_zoneinfoDir = zoneinfoDir;
        m_zoneTab = zoneTab;
        return true;
    }

    zoneinfoDir = ::getenv("TZDIR");
    zoneTab = zoneinfoDir + ZONE_TAB_FILE;
    if (!zoneinfoDir.isEmpty() && dir.exists(zoneinfoDir) && QFileInfo::exists(zoneTab)) {
        m_zoneinfoDir = zoneinfoDir;
        m_zoneTab = zoneTab;
        return true;
    }

    zoneinfoDir = QLatin1String("/usr/share/lib/zoneinfo");
    zoneTab = zoneinfoDir + ZONE_TAB_FILE;
    if (dir.exists(zoneinfoDir + QLatin1String("/src")) && QFileInfo::exists(zoneTab)) {
        m_zoneinfoDir = zoneinfoDir;
        m_zoneTab = zoneTab;
        return true;
    }

    return false;
}

void KTimeZoned::zonetabChanged()
{
    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/Daemon"), QStringLiteral("org.kde.KTimeZoned"), QStringLiteral("timeZoneDatabaseUpdated"));
    QDBusConnection::sessionBus().send(message);
}

#include "ktimezoned.moc"
#include "moc_ktimezonedbase.cpp"
