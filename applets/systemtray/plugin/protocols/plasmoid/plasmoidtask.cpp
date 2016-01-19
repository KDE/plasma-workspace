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
#include <QAction>
#include <QQuickWindow>
#include <kdeclarative/qmlobject.h>

#include <KActionCollection>
#include <KPluginInfo>
#include <KLocalizedString>

#include <QLoggingCategory>
#include <QMenu>

#include <Plasma/Applet>
#include <Plasma/PluginLoader>

#include "appletquickitem.h"
#include "config-workspace.h"

namespace SystemTray
{

PlasmoidTask::PlasmoidTask(const QString &packageName, int appletId, Plasma::Containment *cont, QObject *parent)
    : Task(parent),
      m_taskId(packageName),
      m_applet(0),
      m_valid(false)
{
    qCDebug(SYSTEMTRAY) << "Loading applet: " << packageName << appletId;

    m_applet = Plasma::PluginLoader::self()->loadApplet(packageName, appletId);
    if (!m_applet) {
        qWarning() << "could not load plasma applet " << packageName << appletId;
        return;
    }

    m_valid = true;

    cont->setImmutability(Plasma::Types::Mutable);
    cont->addApplet(m_applet);
    m_applet->setParent(cont);
    //FIXME? This is *maybe* not necessary
    m_applet->init();

    m_taskGraphicsObject = m_applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();

    if (m_taskGraphicsObject) {
        /*override any default size for applets, in the systray the size is NEVER
         * controlled by applet itself. This caused some plasmoids like
         * klipper getting expanded for an instant before collapsing again
         */
        m_taskGraphicsObject->setWidth(0);
        m_taskGraphicsObject->setHeight(0);

        Plasma::Package package = Plasma::PluginLoader::self()->loadPackage(QStringLiteral("Plasma/Shell"));
        package.setDefaultPackageRoot(PLASMA_RELATIVE_DATA_INSTALL_DIR "/plasmoids/");
        package.setPath(QStringLiteral("org.kde.plasma.systemtray"));

        m_taskGraphicsObject->setCoronaPackage(package);
        QMetaObject::invokeMethod(m_taskGraphicsObject, "init", Qt::QueuedConnection);

        //old syntax, because we are connecting blindly
        connect(m_taskGraphicsObject, &PlasmaQuick::AppletQuickItem::expandedChanged,
                this, &Task::expandedChanged);
    }



    if (!m_applet) {
        qCDebug(SYSTEMTRAY) << "Invalid applet taskitem";
        m_valid = false;
        return;
    }
    connect(m_applet, &Plasma::Applet::statusChanged, this, &PlasmoidTask::updateStatus);

    if (pluginInfo().isValid()) {
        setName(pluginInfo().name());
        m_iconName = pluginInfo().icon();

        const QString category = pluginInfo().property(QStringLiteral("X-Plasma-NotificationAreaCategory")).toString();
        if (!category.isEmpty()) {

            int index = metaObject()->indexOfEnumerator("Category");
            int key = metaObject()->enumerator(index).keyToValue(category.toLatin1());

            if (key != -1) {
                setCategory(static_cast<Task::Category>(key));
            }
        }
    } else {
        qWarning() << "Invalid Plasmoid: " << packageName;
    }
    updateStatus();
}

PlasmoidTask::~PlasmoidTask()
{
    if (m_applet) {
        m_applet->destroy();
    }
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
    } else if (ps == Plasma::Types::HiddenStatus) {
        setStatus(Task::HiddenStatus);
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

QKeySequence PlasmoidTask::shortcut() const
{
    return m_applet->globalShortcut();
}

void PlasmoidTask::setShortcut(const QKeySequence &sequence)
{
    if (m_applet->globalShortcut() != sequence) {
        m_applet->setGlobalShortcut(sequence);
        emit changedShortcut();
    }
}

void PlasmoidTask::configure()
{
    if (!m_applet) {
        return;
    }

    m_applet->actions()->action(QStringLiteral("configure"))->trigger();
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

void PlasmoidTask::showMenu(int x, int y)
{
    QPoint pos(x, y);

    QMenu *desktopMenu = new QMenu;
    connect(this, &QObject::destroyed, desktopMenu, &QMenu::close);
    desktopMenu->setAttribute(Qt::WA_DeleteOnClose);

    foreach (QAction *action, m_applet->contextualActions()) {
        if (action) {
            desktopMenu->addAction(action);
        }
    }

    QAction *runAssociatedApplication = m_applet->actions()->action(QStringLiteral("run associated application"));
    if (runAssociatedApplication && runAssociatedApplication->isEnabled()) {
        desktopMenu->addAction(runAssociatedApplication);
    }

    if (m_applet->actions()->action(QStringLiteral("configure"))) {
        desktopMenu->addAction(m_applet->actions()->action(QStringLiteral("configure")));
    }


    if (parent() && parent()->parent()) {
        Host* h = qobject_cast<Host*>(parent()->parent());
        if (h) {
            QQuickItem* rootItem = h->rootItem();
            if (rootItem) {
                Plasma::Applet *systrayApplet = rootItem->property("_plasma_applet").value<Plasma::Applet*>();

                if (systrayApplet) {
                    QMenu *systrayMenu = new QMenu(i18n("System Tray Options"), desktopMenu);

                    foreach (QAction *action, systrayApplet->contextualActions()) {
                        if (action) {
                            systrayMenu->addAction(action);
                        }
                    }
                    if (systrayApplet->actions()->action(QStringLiteral("configure"))) {
                        systrayMenu->addAction(systrayApplet->actions()->action(QStringLiteral("configure")));
                    }
                    if (systrayApplet->actions()->action(QStringLiteral("remove"))) {
                        systrayMenu->addAction(systrayApplet->actions()->action(QStringLiteral("remove")));
                    }
                    desktopMenu->addMenu(systrayMenu);

                    if (systrayApplet->containment() && status() >= Active) {
                        QMenu *containmentMenu = new QMenu(i18nc("%1 is the name of the containment", "%1 Options", systrayApplet->containment()->title()), desktopMenu);

                        foreach (QAction *action, systrayApplet->containment()->contextualActions()) {
                            if (action) {
                                containmentMenu->addAction(action);
                            }
                        }
                        foreach (QAction *action, systrayApplet->containment()->actions()->actions()) {
                            if (action) {
                                containmentMenu->addAction(action);
                            }
                        }
                        desktopMenu->addMenu(containmentMenu);
                    }
                }
            }
        }
    }

    desktopMenu->adjustSize();
    desktopMenu->popup(pos);
}

Plasma::Applet *PlasmoidTask::applet()
{
    return m_applet;
}

}


