/*
    SPDX-FileCopyrightText: 2009 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ktimezoned_win.moc"
#include "ktimezonedbase.moc"

#include <climits>
#include <cstdlib>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QStringList>
#include <QTextStream>
#include <QThread>

#include <kcodecs.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include <kpluginfactory.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

K_PLUGIN_CLASS_WITH_JSON(KTimeZoned, "ktimezoned.json")

// Config file entry names
const char LOCAL_ZONE[] = "LocalZone"; // name of local time zone
static const TCHAR currentTimeZoneKey[] = TEXT("System\\CurrentControlSet\\Control\\TimeZoneInformation");

class RegistryWatcherThread : public QThread
{
public:
    RegistryWatcherThread(KTimeZoned *parent)
        : QThread(parent)
        , q(parent)
    {
    }

    ~RegistryWatcherThread()
    {
        RegCloseKey(key);
    }

    void run()
    {
// FIXME: the timezonechange needs to be handled differently
#ifndef _WIN32
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, currentTimeZoneKey, 0, KEY_READ, &key) == ERROR_SUCCESS) {
            while (true) {
                RegNotifyChangeKeyValue(key, true, REG_NOTIFY_CHANGE_LAST_SET, NULL, false /*async, we want it to block*/);
                q->updateLocalZone();
            }
        }
#endif
    }

private:
    KTimeZoned *q;
    HKEY key;
};

KTimeZoned::KTimeZoned(QObject *parent, const QList<QVariant> &l)
    : KTimeZonedBase(parent, l)
    , mRegistryWatcherThread(0)
{
    init(false);
}

KTimeZoned::~KTimeZoned()
{
    if (mRegistryWatcherThread) {
        mRegistryWatcherThread->quit();
        mRegistryWatcherThread->wait(100);
    }
    delete mRegistryWatcherThread;
}

void KTimeZoned::init(bool restart)
{
    if (restart) {
        qDebug() << "KTimeZoned::init(restart)";
        delete mRegistryWatcherThread;
        mRegistryWatcherThread = 0;
    }

    KConfig config(QLatin1String("ktimezonedrc"));
    if (restart)
        config.reparseConfiguration();
    KConfigGroup group(&config, "TimeZones");
    mConfigLocalZone = group.readEntry(LOCAL_ZONE);

    updateLocalZone();
    if (!mRegistryWatcherThread) {
        mRegistryWatcherThread = new RegistryWatcherThread(this);
        mRegistryWatcherThread->start();
    }
}

// Check if the local zone has been updated, and if so, write the new
// zone to the config file and notify interested parties.
void KTimeZoned::updateLocalZone()
{
    // On Windows, HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Time Zones
    // holds the time zone database. The TZI binary value is the TIME_ZONE_INFORMATION structure.

    TIME_ZONE_INFORMATION tzinfo;
    DWORD res = GetTimeZoneInformation(&tzinfo);
    if (res == TIME_ZONE_ID_INVALID)
        return; // hm
    mLocalZone = QString::fromUtf16(reinterpret_cast<ushort *>(tzinfo.StandardName));

    if (mConfigLocalZone != mLocalZone) {
        qDebug() << "Local timezone is now: " << mLocalZone;
        KConfig config(QLatin1String("ktimezonedrc"));
        KConfigGroup group(&config, "TimeZones");
        mConfigLocalZone = mLocalZone;
        group.writeEntry(LOCAL_ZONE, mConfigLocalZone);
        group.sync();

        QDBusMessage message = QDBusMessage::createSignal("/Daemon", "org.kde.KTimeZoned", "timeZoneChanged");
        QDBusConnection::sessionBus().send(message);
    }
}
