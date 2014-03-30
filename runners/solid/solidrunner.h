/***************************************************************************
 *   Copyright 2009 by Jacopo De Simoi <wilderkde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/
#ifndef SOLIDRUNNER_H
#define SOLIDRUNNER_H

#include <Plasma/AbstractRunner>

namespace Plasma {
    class DataEngine;
    class DataEngineManager;
    class RunnerContext;
}

class DeviceWrapper;

class SolidRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

    public:
        SolidRunner(QObject* parent, const QVariantList &args);
        ~SolidRunner();

        virtual void match(Plasma::RunnerContext& context);
        virtual void run(const Plasma::RunnerContext& context, const Plasma::QueryMatch& match);


    protected:
        QList<QAction*> actionsForMatch(const Plasma::QueryMatch &match);

    protected Q_SLOTS:

        void init();
        void onSourceAdded(const QString &name);
        void onSourceRemoved(const QString &name);
   
    private Q_SLOTS:
        void registerAction(QString &id, QString icon, QString text, QString desktop);
        void refreshMatch(QString &id);

    private:

        void fillPreviousDevices();
        void cleanActionsForDevice(DeviceWrapper *);
        void createOrUpdateMatches(const QStringList &udiList);

        Plasma::QueryMatch deviceMatch(DeviceWrapper * device);

        Plasma::DataEngine *m_hotplugEngine;
        Plasma::DataEngine *m_solidDeviceEngine;

        QHash<QString, DeviceWrapper*> m_deviceList;
        QStringList m_udiOrderedList;
        Plasma::DataEngineManager* m_engineManager;
        Plasma::RunnerContext m_currentContext;
};

K_EXPORT_PLASMA_RUNNER(solid, SolidRunner)

#endif // SOLIDRUNNER_H
