/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef MAIN_H
#define MAIN_H

#include <kservice.h>

#include <QSet>
#include <QCommandLineParser>

class KCmdLineArgs;

class KCMInit : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KCMInit")
	public Q_SLOTS: //dbus
        Q_SCRIPTABLE void runPhase1();
        Q_SCRIPTABLE void runPhase2();
    Q_SIGNALS: //dbus signal
	Q_SCRIPTABLE void phase1Done();
	Q_SCRIPTABLE void phase2Done();
    public:
        KCMInit( const QCommandLineParser& args );
        ~KCMInit() override;
    private:
        bool runModule(const QString &libName, KService::Ptr service);
        void runModules( int phase );
        KService::List list;
        QSet<QString> alreadyInitialized;
};

#endif // MAIN_H
