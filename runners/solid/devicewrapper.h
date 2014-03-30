/**************************************************************************
 *   Copyright 2009 by Jacopo De Simoi <wilderkde@gmail.com>               *
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

#ifndef DEVICEWRAPPER_H
#define DEVICEWRAPPER_H

#include <QString>

#include <KIcon>

#include <Plasma/DataEngine>

#include <Solid/Device>


#include <solid/solidnamespace.h>

class DeviceWrapper : public QObject
{
    Q_OBJECT

    public:
    DeviceWrapper(const QString& udi);
    ~DeviceWrapper();

    QString id() const;
    Solid::Device device() const;
    KIcon icon() const;
    bool isStorageAccess() const;
    bool isAccessible() const;
    bool isOpticalDisc() const;
    bool isEncryptedContainer() const;
    QString description() const;
    QString defaultAction() const;
    void runAction(QAction *) ;
    QStringList actionIds() const;
    void setForceEject(bool force);

    Q_SIGNALS:
    void registerAction(QString &id, QString icon, QString text, QString desktop);
    void refreshMatch(QString &id);

    protected Q_SLOTS:

    /**
    * slot called when a source of the hotplug engine is updated
    * @param source the name of the source
    * @param data the data of the source
    *
    * @internal
    **/
    void dataUpdated(const QString &source, Plasma::DataEngine::Data data);

    private:

    Solid::Device m_device;
    QString m_iconName;
    bool m_isStorageAccess;
    bool m_isAccessible;
    bool m_isEncryptedContainer;
    bool m_isOpticalDisc;
    bool m_forceEject;
    QString m_description;
    QStringList m_actionIds;
    // Solid doesn't like multithreading that much
    // We cache the informations we need locally so that
    // 1) nothing possibly goes wrong when processing a query
    // 2) performance++

    QString m_udi;
    QStringList m_emblems;
};

#endif //DEVICEWRAPPER_H
