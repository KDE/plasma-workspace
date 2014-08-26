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

#include "plasmawindowedcorona.h"
#include "plasmawindowedview.h"
#include <QDebug>
#include <QAction>
#include <QQuickItem>

#include <KActionCollection>
#include <Plasma/Package>
#include <Plasma/PluginLoader>

PlasmaWindowedCorona::PlasmaWindowedCorona(QObject *parent)
    : Plasma::Corona(parent),
      m_containment(0)
{
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Shell");
    package.setPath("org.kde.plasma.desktop");
    setPackage(package);
    //QMetaObject::invokeMethod(this, "load", Qt::QueuedConnection);
    load();
}

void PlasmaWindowedCorona::loadApplet(const QString &applet)
{
    //view->setContainment(m_containment);
    //view->show();
   // m_views << view;

    PlasmaWindowedView *v = new PlasmaWindowedView();
    v->show();

    Plasma::Containment *cont = containments().first();
    KConfigGroup appletsGroup(config(), "StoredApplets");
    QString plugin;
    for (const QString &group : appletsGroup.groupList()) {
        KConfigGroup cg(&appletsGroup, group);
        plugin = cg.readEntry("plugin", QString());

        if (plugin == applet) {
            Plasma::Applet *a = Plasma::PluginLoader::self()->loadApplet(applet, group.toInt());
            a->restore(cg);

            KConfigGroup cg2 = a->config();
            cg = KConfigGroup(&cg, "Configuration");
            cg.copyTo(&cg2);
            cg = KConfigGroup(&cg, "General");
            cg2 = KConfigGroup(&cg2, "General");
            cont->addApplet(a);

            v->setApplet(a);
            return;
        }
    }

    Plasma::Applet *a = containments().first()->createApplet(applet);
    v->setApplet(a);
}

void PlasmaWindowedCorona::activateRequested(const QStringList &arguments, const QString &workingDirectory)
{
    if (!arguments.count() > 1) {
        return;
    }

    loadApplet(arguments[1]);
}

QRect PlasmaWindowedCorona::screenGeometry(int id) const
{
    Q_UNUSED(id);
return QRect();
    if(m_views.count() > id ) {
        return m_views[id]->geometry();
    } else {
        return QRect();
    }
}

void PlasmaWindowedCorona::load()
{
    KSharedConfig::Ptr c = KSharedConfig::openConfig("plasmawindowed-appletsrc", KConfig::SimpleConfig);
    KConfigGroup conf(c, "Containments");
    conf = KConfigGroup(&conf, "1");
    conf = KConfigGroup(&conf, "Applets");
    KConfigGroup conf2(c, "StoredApplets");
    conf.copyTo(&conf2);
    conf.deleteGroup();
    conf.sync();
    loadLayout("plasmawindowed-appletsrc");


    bool found = false;
    for (auto c : containments()) {
        if (c->containmentType() == Plasma::Types::DesktopContainment) {
            found = true;
            break;
        }
    }

    if (!found) {
        qDebug() << "Loading default layout";
        createContainment("empty"); 
        saveLayout("plasmawindowed-appletsrc");
    }

    for (auto c : containments()) {
        qDebug() << "here we are!";
        if (c->containmentType() == Plasma::Types::DesktopContainment) {
            m_containment = c;
            QAction *removeAction = c->actions()->action("remove");
            if(removeAction) {
                removeAction->deleteLater();
            }
            break;
        }
    }
}


#include "plasmawindowedcorona.moc"
