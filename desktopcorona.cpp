/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>
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
#include <QQmlContext>
#include <QTimer>

#include <KLocalizedString>
#include <Plasma/Package>

#include "containmentconfigview.h"
#include "panelview.h"
#include "view.h"
#include "scripting/desktopscriptengine.h"
#include "widgetexplorer/widgetexplorerview.h"


static const QString s_panelTemplatesPath("plasma-layout-templates/panels/*");

DesktopCorona::DesktopCorona(QObject *parent)
    : Plasma::Corona(parent),
      m_desktopWidget(QApplication::desktop()),
      m_widgetExplorerView(0)
{
    m_desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package().filePath("defaults")), "Desktop");

    m_appConfigSyncTimer = new QTimer(this);
    m_appConfigSyncTimer->setSingleShot(true);
    connect(m_appConfigSyncTimer, &QTimer::timeout,
            this, &DesktopCorona::syncAppConfig);

    connect(m_desktopWidget, SIGNAL(resized(int)),
            this, SLOT(screenResized(int)));
    connect(m_desktopWidget, SIGNAL(screenCountChanged(int)),
            this, SLOT(screenCountChanged(int)));
    connect(m_desktopWidget, SIGNAL(workAreaResized(int)),
            this, SLOT(workAreaResized(int)));
    connect(this, &DesktopCorona::containmentAdded, this, &DesktopCorona::handleContainmentAdded);

    connect(this, SIGNAL(screenOwnerChanged(int, int, Plasma::Containment *)),
            this, SLOT(updateScreenOwner(int, int, Plasma::Containment *)));
    checkViews();

    //QTimer::singleShot(600, this, SLOT(showWidgetExplorer())); // just for easier debugging
}

DesktopCorona::~DesktopCorona()
{
}

KSharedConfig::Ptr DesktopCorona::applicationConfig()
{
    return KSharedConfig::openConfig();
}

void DesktopCorona::requestApplicationConfigSync()
{
    // constant controlling how long between requesting a configuration sync
    // and one happening should occur. currently 10 seconds
    static const int CONFIG_SYNC_TIMEOUT = 10000;

    m_appConfigSyncTimer->start(CONFIG_SYNC_TIMEOUT);
}

void DesktopCorona::loadDefaultLayout()
{
    WorkspaceScripting::DesktopScriptEngine scriptEngine(this, true);
    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError,
            this, &DesktopCorona::printScriptError);
    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print,
            this, &DesktopCorona::printScriptMessage);

    QString script = package().filePath("defaultlayout");

    QFile file(script);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        QString code = file.readAll();
        qDebug() << "evaluating startup script:" << script;
        scriptEngine.evaluateScript(code);
    }
}

void DesktopCorona::processUpdateScripts()
{
    foreach (const QString &script, WorkspaceScripting::ScriptEngine::pendingUpdateScripts(this)) {
        WorkspaceScripting::DesktopScriptEngine scriptEngine(this, false);
        scriptEngine.evaluateScript(script);
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

            Plasma::Types::ContainmentType t = c->containmentType();
            if (t == Plasma::Types::PanelContainment ||
                t == Plasma::Types::CustomPanelContainment) {
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
        c = createContainment(m_desktopDefaultsConfig.readEntry("Containment", "org.kde.desktopcontainment"));
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
    requestApplicationConfigSync();

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
        qDebug() << "TODO: Implement loading containments into the views";
    }
}


void DesktopCorona::checkLoadingDesktopsComplete()
{
    Plasma::Containment *c = qobject_cast<Plasma::Containment *>(sender());
    if (c) {
        disconnect(c, &Plasma::Containment::uiReadyChanged,
                   this, &DesktopCorona::checkLoadingDesktopsComplete);
        m_loadingDesktops.remove(c);
    }

    if (m_loadingDesktops.isEmpty()) {
        foreach (Plasma::Containment *cont, m_waitingPanels) {
            m_panelViews[cont] = new PanelView(this);
            m_panelViews[cont]->setContainment(cont);
        }
        m_waitingPanels.clear();
    }
}

void DesktopCorona::updateScreenOwner(int wasScreen, int isScreen, Plasma::Containment *containment)
{
    qDebug() << "Was screen" << wasScreen << "Is screen" << isScreen <<"Containment" << containment << containment->title();

    if (containment->formFactor() == Plasma::Types::Horizontal ||
        containment->formFactor() == Plasma::Types::Vertical) {

        if (isScreen >= 0) {
            m_waitingPanels << containment;
        } else {
            if (m_panelViews.contains(containment)) {
                m_panelViews[containment]->setContainment(0);
                m_panelViews[containment]->deleteLater();
                m_panelViews.remove(containment);
            }
        }
        checkLoadingDesktopsComplete();

    //Desktop view
    } else {

        if (containment->isUiReady()) {
            checkLoadingDesktopsComplete();
        } else {
            m_loadingDesktops.insert(containment);
            connect(containment, &Plasma::Containment::uiReadyChanged,
                    this, &DesktopCorona::checkLoadingDesktopsComplete);
        }

        if (isScreen < 0 || m_views.count() < isScreen + 1) {
            qWarning() << "Invalid screen";
            return;
        }

        m_views[isScreen]->setContainment(containment);
    }
}

void DesktopCorona::handleContainmentAdded(Plasma::Containment* c)
{
    connect(c, &Plasma::Containment::showAddWidgetsInterface,
            this, &DesktopCorona::showWidgetExplorer);
}

void DesktopCorona::showWidgetExplorer()
{
    if (!m_widgetExplorerView) {

        QString expqml = package().filePath("widgetexplorer");
        qDebug() << "Script to load for WidgetExplorer: " << expqml;
        m_widgetExplorerView = new WidgetExplorerView(expqml);
        m_widgetExplorerView->init();
    }
    Plasma::Containment *c = 0;
    c = dynamic_cast<Plasma::Containment*>(sender());
    if (c) {
        qDebug() << "Found containment.";
        m_widgetExplorerView->setContainment(c);
    } else {
        // FIXME: try harder to find a suitable containment?
        qWarning() << "containment not set, don't know where to add the applet.";
    }
    m_widgetExplorerView->show();
}

void DesktopCorona::syncAppConfig()
{
    qDebug() << "Syncing plasma-shellrc config";
    applicationConfig()->sync();
}

void DesktopCorona::printScriptError(const QString &error)
{
    qWarning() << error;
}

void DesktopCorona::printScriptMessage(const QString &message)
{
    qDebug() << message;
}

#include "moc_desktopcorona.cpp"

