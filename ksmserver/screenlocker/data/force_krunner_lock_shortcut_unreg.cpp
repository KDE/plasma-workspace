/********************************************************************
KWin - the KDE window manager
This file is part of the KDE project.

Copyright (C) 2011 Alex Merry <kde@randomguy3.me.uk>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

// see kscreenlocker_locksession-shortcut.upd

#include <QtDBus/QtDBus>

int main( int argc, char* argv[] )
{
    QDBusInterface accelIface(QStringLiteral("org.kde.kglobalaccel"), QStringLiteral("/kglobalaccel"), QStringLiteral("org.kde.KGlobalAccel"));
    QStringList krunnerShortcutId;
    krunnerShortcutId << QLatin1String("krunner") << QLatin1String("Lock Session") << QString() << QString();
    /*
    QDBusReply<QList<int> > reply = accelIface.call("shortcut", krunnerShortcutId);
    int shortcut = -1;
    if (reply.isValid() && reply.value().size() == 1) {
        shortcut = reply.value().at(0);
    }
    */
    accelIface.call(QDBus::NoBlock, QStringLiteral("unRegister"), krunnerShortcutId);
}
