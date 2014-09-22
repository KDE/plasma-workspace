/*
 * Copyright 2014  Bhushan Shah <bhush94@gmail.com>
 * Copyright 2014 Marco Martin <notmart@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "standaloneappcorona.h"
#include "desktopview.h"
#include "activity.h"
#include <QDebug>
#include <QAction>

#include <kactivities/consumer.h>
#include <KActionCollection>
#include <Plasma/Package>
#include <Plasma/PluginLoader>

#include "scripting/scriptengine.h"

StandaloneAppCorona::StandaloneAppCorona(const QString &coronaPlugin, QObject *parent)
    : Plasma::Corona(parent),
      m_coronaPlugin(coronaPlugin),
      m_activityConsumer(new KActivities::Consumer(this)),
      m_view(0)
{
    qmlRegisterUncreatableType<DesktopView>("org.kde.plasma.shell", 2, 0, "DesktopView", "It is not possible to create objects of type DesktopView");

    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Shell");
    package.setPath(m_coronaPlugin);
    setPackage(package);

    m_desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package.filePath("defaults")), "Desktop");

    connect(m_activityConsumer, SIGNAL(currentActivityChanged(QString)), this, SLOT(currentActivityChanged(QString)));
    connect(m_activityConsumer, SIGNAL(activityAdded(QString)), this, SLOT(activityAdded(QString)));
    connect(m_activityConsumer, SIGNAL(activityRemoved(QString)), this, SLOT(activityRemoved(QString)));

    load();
}

QRect StandaloneAppCorona::screenGeometry(int id) const
{
    Q_UNUSED(id);
    if(m_view) {
        return m_view->geometry();
    } else {
        return QRect();
    }
}

void StandaloneAppCorona::load()
{
    loadLayout("plasma-" + m_coronaPlugin + "-appletsrc");

    bool found = false;
    for (auto c : containments()) {
        if (c->containmentType() == Plasma::Types::DesktopContainment) {
            found = true;
            break;
        }
    }

    if (!found) {
        qDebug() << "Loading default layout";
        loadDefaultLayout();
        saveLayout("plasma-" + m_coronaPlugin + "-appletsrc");
    }

    for (auto c : containments()) {
        qDebug() << "containment found";
        if (c->containmentType() == Plasma::Types::DesktopContainment) {
            m_view = new DesktopView(this);
            QAction *removeAction = c->actions()->action("remove");
            if(removeAction) {
                removeAction->deleteLater();
            }
            m_view->setContainment(c);
            m_view->show();
            connect(m_view, &QWindow::visibleChanged, [=](bool visible){
                if (!visible) {
                    deleteLater();
                }
            });
            break;
        }
    }
}

void StandaloneAppCorona::loadDefaultLayout()
{
    const QString script = package().filePath("defaultlayout");
    QFile file(script);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        QString code = file.readAll();
        qDebug() << "evaluating startup script:" << script;

        WorkspaceScripting::ScriptEngine scriptEngine(this);

        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this,
                [](const QString &msg) {
                    qWarning() << msg;
                });
        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this,
                [](const QString &msg) {
                    qDebug() << msg;
                });
        scriptEngine.evaluateScript(code);
    }
}

Plasma::Containment *StandaloneAppCorona::createContainmentForActivity(const QString& activity, int screenNum)
{
    for (Plasma::Containment *cont : containments()) {
        if (cont->activity() == activity) {
            return cont;
        }
    }

    Plasma::Containment *containment = containmentForScreen(screenNum, m_desktopDefaultsConfig.readEntry("Containment", "org.kde.desktopcontainment"), QVariantList());
    Q_ASSERT(containment);

    if (containment) {
        containment->setActivity(activity);
    }

    return containment;
}

void StandaloneAppCorona::activityAdded(const QString &id)
{
    //TODO more sanity checks
    if (m_activities.contains(id)) {
        qWarning() << "Activity added twice" << id;
        return;
    }

    Activity *a = new Activity(id, this);
    m_activities.insert(id, a);
}

void StandaloneAppCorona::activityRemoved(const QString &id)
{
    Activity *a = m_activities.take(id);
    a->deleteLater();
}

void StandaloneAppCorona::currentActivityChanged(const QString &newActivity)
{
//  qDebug() << "Activity changed:" << newActivity;

    Plasma::Containment *c = createContainmentForActivity(newActivity, 0);

    QAction *removeAction = c->actions()->action("remove");
    if (removeAction) {
        removeAction->deleteLater();
    }
    m_view->setContainment(c);

}

void StandaloneAppCorona::insertActivity(const QString &id, Activity *activity)
{
    m_activities.insert(id, activity);
    Plasma::Containment *c = createContainmentForActivity(id, 0);
    if (c) {
        c->config().writeEntry("lastScreen", 0);
    }
}

#include "standaloneappcorona.moc"
