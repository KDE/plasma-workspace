/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
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

#include "desktopcorona.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <Plasma/Package>

#include "panelview.h"
#include "view.h"
#include "scripting/desktopscriptengine.h"


static const QString s_panelTemplatesPath("plasma-layout-templates/panels/*");

DesktopCorona::DesktopCorona(QObject *parent)
    : Plasma::Corona(parent),
      m_desktopWidget(QApplication::desktop())
{
    m_desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package().filePath("defaults")), "Desktop");

    connect(m_desktopWidget, SIGNAL(resized(int)),
            this, SLOT(screenResized(int)));
    connect(m_desktopWidget, SIGNAL(screenCountChanged(int)),
            this, SLOT(screenCountChanged(int)));
    connect(m_desktopWidget, SIGNAL(workAreaResized(int)),
            this, SLOT(workAreaResized(int)));

    connect(this, SIGNAL(screenOwnerChanged(int, int, Plasma::Containment *)),
            this, SLOT(updateScreenOwner(int, int, Plasma::Containment *)));
    checkViews();
}

DesktopCorona::~DesktopCorona()
{
}



void DesktopCorona::loadDefaultLayout()
{
    WorkspaceScripting::DesktopScriptEngine scriptEngine(this, true);
    connect(&scriptEngine, SIGNAL(printError(QString)), this, SLOT(printScriptError(QString)));
    connect(&scriptEngine, SIGNAL(print(QString)), this, SLOT(printScriptMessage(QString)));

    QString script = package().filePath("defaultlayout");

    QFile file(script);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        QString code = file.readAll();
        qDebug() << "evaluating startup script:" << script;
        scriptEngine.evaluateScript(code);
    }
}

void DesktopCorona::checkScreens(bool signalWhenExists)
{
    // quick sanity check to ensure we have containments for each screen
    int num = numScreens();
    for (int i = 0; i < num; ++i) {
        checkScreen(i, signalWhenExists);
    }
}

void DesktopCorona::checkScreen(int screen, bool signalWhenExists)
{
    // signalWhenExists is there to allow PlasmaApp to know when to create views
    // it does this only on containment addition, but in the case of a screen being
    // added and the containment already existing for that screen, no signal is emitted
    // and so PlasmaApp does not know that it needs to create a view for it. to avoid
    // taking care of that case in PlasmaApp (which would duplicate some of the code below,
    // DesktopCorona will, when signalWhenExists is true, emit a containmentAdded signal
    // even if the containment actually existed prior to this method being called.
    //
    //note: hte signal actually triggers view creation only for panels, atm.
    //desktop views are created in response to containment's screenChanged signal instead, which is
    //buggy (sometimes the containment thinks it's already on the screen, so no view is created)

    //TODO: restore activities
    //Activity *currentActivity = activity(m_activityController->currentActivity());
    //ensure the desktop(s) have a containment and view
    checkDesktop(/*currentActivity,*/ signalWhenExists, screen);


    //ensure the panels get views too
    if (signalWhenExists) {
        foreach (Plasma::Containment * c, containments()) {
            if (c->screen() != screen) {
                continue;
            }

            Plasma::ContainmentType t = c->containmentType();
            if (t == Plasma::PanelContainment ||
                t == Plasma::CustomPanelContainment) {
                emit containmentAdded(c);
            }
        }
    }
}

void DesktopCorona::checkDesktop(/*Activity *activity,*/ bool signalWhenExists, int screen)
{
    Plasma::Containment *c = /*activity->*/containmentForScreen(screen);

    //TODO: remove following when activities are restored
    if (!c) {
        c = createContainment(m_desktopDefaultsConfig.readEntry("Containment", "org.kde.testcontainment"));
    }

    if (!c) {
        return;
    }

    c->setScreen(screen);
    if (screen >= 0 || m_views.count() >= screen + 1) {
        m_views[screen]->setContainment(c);
    } else {
        qWarning() << "Invalid screen";
    }
    c->flushPendingConstraintsEvents();
    requestConfigSync();

    if (signalWhenExists) {
        emit containmentAdded(c);
    }
}

int DesktopCorona::numScreens() const
{
    return m_desktopWidget->screenCount();
}

QRect DesktopCorona::screenGeometry(int id) const
{
    return m_desktopWidget->screenGeometry(id);
}

QRegion DesktopCorona::availableScreenRegion(int id) const
{
    return m_desktopWidget->availableGeometry(id);
}

QRect DesktopCorona::availableScreenRect(int id) const
{
    return m_desktopWidget->availableGeometry(id);
}

PanelView *DesktopCorona::panelView(Plasma::Containment *containment) const
{
    return m_panelViews.value(containment);
}


///// SLOTS

void DesktopCorona::screenCountChanged(int newCount)
{
    qDebug() << "New screen count" << newCount;
    checkViews();
}

void DesktopCorona::screenResized(int screen)
{
    qDebug() << "Screen resized" << screen;
}

void DesktopCorona::workAreaResized(int screen)
{
    qDebug() << "Work area resized" << screen;
}

void DesktopCorona::checkViews()
{
    if (m_views.count() == m_desktopWidget->screenCount()) {
        return;
    } else if (m_views.count() < m_desktopWidget->screenCount()) {
        for (int i = m_views.count(); i < m_desktopWidget->screenCount(); ++i) {
            View *view = new View(this);
            QSurfaceFormat format;
            view->init();
            view->show();
            
            m_views << view;
        }
    } else {
        for (int i = m_desktopWidget->screenCount(); i < m_views.count(); ++i) {
            View *view = m_views.last();
            view->deleteLater();
            m_views.pop_back();
        }
    }

    //check every containment is in proper view
    for (int i = 0; i < m_desktopWidget->screenCount(); ++i) {
        
    }
}

void DesktopCorona::updateScreenOwner(int wasScreen, int isScreen, Plasma::Containment *containment)
{
    qDebug() << "Was screen" << wasScreen << "Is screen" << isScreen <<"Containment" << containment << containment->title();

    if (containment->formFactor() == Plasma::Horizontal ||
        containment->formFactor() == Plasma::Vertical) {

        if (isScreen >= 0) {
            m_panelViews[containment] = new PanelView(this);
            m_panelViews[containment]->init();
            m_panelViews[containment]->setContainment(containment);
            m_panelViews[containment]->show();
        } else {
            if (m_panelViews.contains(containment)) {
                m_panelViews[containment]->setContainment(0);
                m_panelViews[containment]->deleteLater();
                m_panelViews.remove(containment);
            }
        }
    
    //Desktop view
    } else {
    
        if (isScreen < 0 || m_views.count() < isScreen + 1) {
            qWarning() << "Invalid screen";
            return;
        }

        m_views[isScreen]->setContainment(containment);
    }
}

#include "desktopcorona.moc"

