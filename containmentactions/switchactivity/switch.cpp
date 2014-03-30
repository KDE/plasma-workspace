/*
 *   Copyright 2009 by Chani Armitage <chani@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "switch.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>

#include <QDebug>
#include <KMenu>
#include <KIcon>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/ServiceJob>
#include <Plasma/DataEngine>

Q_DECLARE_METATYPE(QWeakPointer<Plasma::Containment>)

SwitchActivity::SwitchActivity(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args),
      m_menu(new KMenu()),
      m_action(new QAction(this))
{
    //This is an awful hack, but I need to keep the old behaviour for plasma-netbook
    //while using the new activity API for plasma-desktop.
    //TODO 4.6 convert netbook to the activity API so we won't need this
    m_useNepomuk = (qApp->applicationName() == "plasma-desktop");

    connect(m_menu, SIGNAL(triggered(QAction*)), this, SLOT(switchTo(QAction*)));

    m_action->setMenu(m_menu);
    m_menu->setTitle(i18n("Activities"));
}

SwitchActivity::~SwitchActivity()
{
    delete m_menu;
}

void SwitchActivity::contextEvent(QEvent *event)
{
    switch (event->type()) {
        case QEvent::GraphicsSceneMousePress:
            contextEvent(static_cast<QGraphicsSceneMouseEvent*>(event));
            break;
        case QEvent::GraphicsSceneWheel:
            wheelEvent(static_cast<QGraphicsSceneWheelEvent*>(event));
            break;
        default:
            break;
    }
}

void SwitchActivity::makeMenu()
{
    m_menu->clear();
    m_menu->addTitle(i18n("Activities"));

    if (m_useNepomuk) {
        Plasma::DataEngine *engine = dataEngine("org.kde.activities");
        if (!engine->isValid()) {
            return;
        }

        Plasma::DataEngine::Data data = engine->query("Status");
        QStringList activities = data["Running"].toStringList();
        foreach (const QString& id, activities) {
            Plasma::DataEngine::Data data = engine->query(id);
            QAction *action = m_menu->addAction(KIcon(data["Icon"].toString()), data["Name"].toString());
            action->setData(QVariant(id));
            if (data["Current"].toBool()) {
                action->setEnabled(false);
            }
        }
    } else {
        Plasma::Containment *myCtmt = containment();
        if (!myCtmt) {
            return;
        }

        Plasma::Corona *c = myCtmt->corona();
        if (!c) {
            return;
        }

        QList<Plasma::Containment*> containments = c->containments();
        foreach (Plasma::Containment *ctmt, containments) {
            if (ctmt->containmentType() == Plasma::Containment::PanelContainment ||
                    ctmt->containmentType() == Plasma::Containment::CustomPanelContainment ||
                    c->offscreenWidgets().contains(ctmt)) {
                continue;
            }

            QString name = ctmt->activity();
            if (name.isEmpty()) {
                name = ctmt->name();
            }
            QAction *action = m_menu->addAction(name);
            action->setData(QVariant::fromValue<QWeakPointer<Plasma::Containment> >(QWeakPointer<Plasma::Containment>(ctmt)));

            //WARNING this assumes the plugin will only ever be set on activities, not panels!
            if (ctmt == myCtmt) {
                action->setEnabled(false);
            }
        }
    }

    m_menu->adjustSize();
}

void SwitchActivity::contextEvent(QGraphicsSceneMouseEvent *event)
{
    makeMenu();
    m_menu->exec(popupPosition(m_menu->size(), event));
}

QList<QAction*> SwitchActivity::contextualActions()
{
    makeMenu();

    QList<QAction*> list;
    list << m_action;
    return list;
}

void SwitchActivity::switchTo(QAction *action)
{
    if (m_useNepomuk) {
        QString id = action->data().toString();
        Plasma::Service *service = dataEngine("org.kde.activities")->serviceForSource(id);
        KConfigGroup op = service->operationDescription("setCurrent");
        Plasma::ServiceJob *job = service->startOperationCall(op);
        connect(job, SIGNAL(finished(KJob*)), service, SLOT(deleteLater()));
    } else {
        QWeakPointer<Plasma::Containment> ctmt = action->data().value<QWeakPointer<Plasma::Containment> >();
        if (!ctmt) {
            return;
        }
        Plasma::Containment *myCtmt = containment();
        if (!myCtmt) {
            return;
        }

        ctmt.data()->setScreen(myCtmt->screen(), myCtmt->desktop());
    }
}

void SwitchActivity::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    int step = (event->delta() < 0) ? 1 : -1;

    if (m_useNepomuk) {
        Plasma::DataEngine *engine = dataEngine("org.kde.activities");
        if (!engine->isValid()) {
            return;
        }

        Plasma::DataEngine::Data data = engine->query("Status");
        QStringList list = data["Running"].toStringList();
        QString current = data["Current"].toString();
        int start = list.indexOf(current);
        int next = (start + step + list.size()) % list.size();

        Plasma::Service *service = engine->serviceForSource(list.at(next));
        KConfigGroup op = service->operationDescription("setCurrent");
        Plasma::ServiceJob *job = service->startOperationCall(op);
        connect(job, SIGNAL(finished(KJob*)), service, SLOT(deleteLater()));
        return;
    }

    Plasma::Containment *myCtmt = containment();
    if (!myCtmt) {
        return;
    }

    Plasma::Corona *c = myCtmt->corona();
    if (!c) {
        return;
    }

    QList<Plasma::Containment*> containments = c->containments();
    int start = containments.indexOf(myCtmt);
    int i = (start + step + containments.size()) % containments.size();

    //FIXME we *really* need a proper way to cycle through activities
    while (i != start) {
        Plasma::Containment *ctmt = containments.at(i);
        if (ctmt->containmentType() == Plasma::Containment::PanelContainment ||
            ctmt->containmentType() == Plasma::Containment::CustomPanelContainment ||
            c->offscreenWidgets().contains(ctmt)) {
            //keep looking
            i = (i + step + containments.size()) % containments.size();
        } else {
            break;
        }
    }

    Plasma::Containment *ctmt = containments.at(i);
    if (ctmt && ctmt != myCtmt) {
        ctmt->setScreen(myCtmt->screen(), myCtmt->desktop());
    }
}


#include "switch.moc"
