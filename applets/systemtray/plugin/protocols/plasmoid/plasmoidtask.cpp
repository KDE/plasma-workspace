/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
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

#include "plasmoidtask.h"
#include "plasmoidprotocol.h"
#include "../../host.h"
#include "debug.h"

#include <QtCore/QMetaEnum>
#include <kdeclarative/qmlobject.h>

#include <KPluginInfo>

#include <QLoggingCategory>

#include <Plasma/Applet>
#include <Plasma/PluginLoader>

#include "appletquickitem.h"

namespace SystemTray
{

PlasmoidTask::PlasmoidTask(const QString &packageName, int appletId, Plasma::Containment *cont, QObject *parent)
    : Task(parent),
      m_taskId(packageName),
      m_applet(0),
      m_valid(true)
{
    qCDebug(SYSTEMTRAY) << "Loading applet: " << packageName << appletId;

    m_applet = Plasma::PluginLoader::self()->loadApplet(packageName, appletId);
    cont->addApplet(m_applet);
    //FIXME? This is *maybe* not necessary
    m_applet->init();

    m_taskGraphicsObject = m_applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();

    if (m_taskGraphicsObject) {
        Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Shell");
        package.setDefaultPackageRoot("plasma/plasmoids/");
        package.setPath("org.kde.plasma.systemtray");

        m_taskGraphicsObject->setCoronaPackage(package);
        QMetaObject::invokeMethod(m_taskGraphicsObject, "init", Qt::DirectConnection);
        qWarning()<<m_taskGraphicsObject->property("compactRepresentationItem");
        qWarning()<<m_taskGraphicsObject->property("fullRepresentationItem");

        //old syntax, because we are connecting blindly
        connect(m_taskGraphicsObject, SIGNAL(expandedChanged(bool)),
                this, SIGNAL(expandedChanged(bool)));
    }



    if (!m_applet) {
        qCDebug(SYSTEMTRAY) << "Invalid applet taskitem";
        m_valid = false;
        return;
    }
    connect(m_applet, &Plasma::Applet::statusChanged, this, &PlasmoidTask::updateStatus);

    if (pluginInfo().isValid()) {
        setName(pluginInfo().name());
    } else {
        qWarning() << "Invalid Plasmoid: " << packageName;
    }
    updateStatus();
}

PlasmoidTask::~PlasmoidTask()
{
}

KPluginInfo PlasmoidTask::pluginInfo() const
{
    if (!m_applet) {
        return KPluginInfo();
    }
    return m_applet->pluginInfo();
}

void PlasmoidTask::updateStatus()
{
    if (!m_applet || !pluginInfo().isValid()) {
        return;
    }
    const Plasma::Types::ItemStatus ps = m_applet->status();
    if (ps == Plasma::Types::UnknownStatus) {
        setStatus(Task::UnknownStatus);
    } else if (ps == Plasma::Types::PassiveStatus) {
        setStatus(Task::Passive);
    } else if (ps == Plasma::Types::NeedsAttentionStatus) {
        setStatus(Task::NeedsAttention);
    } else {
        setStatus(Task::Active);
    }
}

bool PlasmoidTask::isValid() const
{
    return m_valid && pluginInfo().isValid();
}

bool PlasmoidTask::isEmbeddable() const
{
    return false; // this task cannot be embed because it only provides information to GUI part
}

bool PlasmoidTask::isWidget() const
{
    return false; // isn't a widget
}

void PlasmoidTask::setShortcut(QString text) {
    if (m_shortcut != text) {
        m_shortcut = text;
        emit changedShortcut();
    }
}

void PlasmoidTask::setLocation(Plasma::Types::Location loc)
{
    if (m_applet) {
//        m_applet->setLocation(loc);
    }
}

QString PlasmoidTask::taskId() const
{
    return m_taskId;
}

QQuickItem* PlasmoidTask::taskItem()
{
    if (m_taskGraphicsObject) {
        return m_taskGraphicsObject;
    }
    //FIXME
    return new QQuickItem();//m_applet;
}

QQuickItem* PlasmoidTask::taskItemExpanded()
{
    if (!m_applet) {
        return 0;
    }

    if (m_taskGraphicsObject && m_taskGraphicsObject->property("fullRepresentationItem").value<QQuickItem *>()) {
        return m_taskGraphicsObject->property("fullRepresentationItem").value<QQuickItem *>();
    }
    //FIXME
    return new QQuickItem();//m_applet->defaultRepresentation();
}

QIcon PlasmoidTask::icon() const
{
    return m_icon;
}
//Status

void PlasmoidTask::syncStatus(QString newStatus)
{
    Task::Status status = (Task::Status)metaObject()->enumerator(metaObject()->indexOfEnumerator("Status")).keyToValue(newStatus.toLatin1());

    if (this->status() == status) {
        return;
    }

    setStatus(status);
}

bool PlasmoidTask::expanded() const
{
    if (m_taskGraphicsObject) {
        return m_taskGraphicsObject->property("expanded").toBool();
    } else {
        return false;
    }
}

void PlasmoidTask::setExpanded(bool expanded)
{
    if (m_taskGraphicsObject) {
        m_taskGraphicsObject->setProperty("expanded", expanded);
    }
}


}

#include "plasmoidtask.moc"
