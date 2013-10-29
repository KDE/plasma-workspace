/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *   Copyright 2013 Ivan Cukic <ivan.cukic@kde.org>
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

#include "shellcorona.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QQmlContext>
#include <QTimer>

#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <Plasma/Package>
#include <Plasma/PluginLoader>
#include <kactivities/controller.h>
#include <kactivities/consumer.h>


#include "activity.h"
#include "desktopview.h"
#include "panelview.h"
#include "scripting/desktopscriptengine.h"
#include "widgetexplorer/widgetexplorer.h"
#include "configview.h"

class ShellCorona::Private {
public:
    Private(ShellCorona *corona)
        : q(corona),
          desktopWidget(QApplication::desktop()),
          widgetExplorer(nullptr),
          activityController(new KActivities::Controller(q)),
          activityConsumer(new KActivities::Consumer(q))
    {
        appConfigSyncTimer.setSingleShot(true);
        // constant controlling how long between requesting a configuration sync
        // and one happening should occur. currently 10 seconds
        appConfigSyncTimer.setInterval(10000);

        waitingPanelsTimer = new QTimer(q);
        waitingPanelsTimer->setSingleShot(true);
        waitingPanelsTimer->setInterval(250);
        connect(waitingPanelsTimer, &QTimer::timeout, q, &ShellCorona::createWaitingPanels);
    }

    ShellCorona *q;
    QString shell;
    QDesktopWidget * desktopWidget;
    QList <DesktopView *> views;
    QPointer<WidgetExplorer> widgetExplorer;
    KActivities::Controller *activityController;
    KActivities::Consumer *activityConsumer;
    QHash <Plasma::Containment *, PanelView *> panelViews;
    KConfigGroup desktopDefaultsConfig;
    WorkspaceScripting::DesktopScriptEngine * scriptEngine;
    QList<Plasma::Containment *> waitingPanels;
    QSet<Plasma::Containment *> loadingDesktops;
    QHash<QString, Activity*> activities;
    QTimer *waitingPanelsTimer;

    QTimer appConfigSyncTimer;
};

WorkspaceScripting::DesktopScriptEngine * ShellCorona::scriptEngine() const
{
    return d->scriptEngine;
}


ShellCorona::ShellCorona(QObject *parent)
    : Plasma::Corona(parent),
      d(new Private(this))
{
    d->desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package().filePath("defaults")), "Desktop");

    qmlRegisterType<WidgetExplorer>("org.kde.plasma.private.shell", 2, 0, "WidgetExplorer");
    qmlRegisterType<DesktopView>();

    connect(&d->appConfigSyncTimer, &QTimer::timeout,
            this, &ShellCorona::syncAppConfig);

    connect(d->desktopWidget, &QDesktopWidget::resized,
            this, &ShellCorona::screenResized );
    connect(d->desktopWidget, &QDesktopWidget::screenCountChanged,
            this, &ShellCorona::screenCountChanged);
    connect(d->desktopWidget, &QDesktopWidget::workAreaResized,
            this, &ShellCorona::workAreaResized);

    connect(this, &ShellCorona::containmentAdded,
            this, &ShellCorona::handleContainmentAdded);
    connect(this, &ShellCorona::screenOwnerChanged,
            this, &ShellCorona::updateScreenOwner);

    d->scriptEngine = new WorkspaceScripting::DesktopScriptEngine(this, true);

    connect(d->scriptEngine, &WorkspaceScripting::ScriptEngine::printError,
            this, &ShellCorona::printScriptError);
    connect(d->scriptEngine, &WorkspaceScripting::ScriptEngine::print,
            this, &ShellCorona::printScriptMessage);

    QAction *dashboardAction = actions()->add<QAction>("show dashboard");
    QObject::connect(dashboardAction, &QAction::triggered,
                     this, &ShellCorona::setDashboardShown);
    dashboardAction->setText(i18n("Show Dashboard"));
    dashboardAction->setAutoRepeat(true);
    dashboardAction->setCheckable(true);
    dashboardAction->setIcon(QIcon::fromTheme("dashboard-show"));
    dashboardAction->setData(Plasma::Types::ControlAction);
    dashboardAction->setShortcut(QKeySequence("ctrl+f12"));
    dashboardAction->setShortcutContext(Qt::ApplicationShortcut);

    //Activity stuff

    QAction *activityAction = actions()->addAction("manage activities");
    connect(activityAction, &QAction::triggered,
            this, &ShellCorona::toggleActivityManager);
    activityAction->setText(i18n("Activities..."));
    activityAction->setIcon(QIcon::fromTheme("preferences-activities"));
    activityAction->setData(Plasma::Types::ConfigureAction);
    activityAction->setShortcut(QKeySequence("alt+d, alt+a"));
    activityAction->setShortcutContext(Qt::ApplicationShortcut);

    connect(this, SIGNAL(immutabilityChanged(Plasma::ImmutabilityType)),
            this, SLOT(updateImmutability(Plasma::ImmutabilityType)));

    connect(d->activityConsumer, SIGNAL(currentActivityChanged(QString)), this, SLOT(currentActivityChanged(QString)));
    connect(d->activityConsumer, SIGNAL(activityAdded(QString)), this, SLOT(activityAdded(QString)));
    connect(d->activityConsumer, SIGNAL(activityRemoved(QString)), this, SLOT(activityRemoved(QString)));
}

ShellCorona::~ShellCorona()
{
}

void ShellCorona::setShell(const QString &shell)
{
    if (d->shell == shell) return;

    unload();

    d->shell = shell;
    KConfigGroup config(KSharedConfig::openConfig(), "General");
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Shell");
    package.setPath(shell);
    setPackage(package);

    load();
    //TODO: panel views should be synced here: either creating views for panels without, or deleting views for panels that don't have one anymore
}

QString ShellCorona::shell() const
{
    return d->shell;
}

void ShellCorona::load()
{
    if (d->shell.isEmpty()) return;

    checkViews();
    loadLayout("plasma-" + d->shell + "-appletsrc");

    if (containments().isEmpty()) {
        loadDefaultLayout();
    }

    processUpdateScripts();
    checkActivities();
    checkScreens();
}

void ShellCorona::unload()
{
    if (d->shell.isEmpty()) return;

    qDeleteAll(containments());
}

KSharedConfig::Ptr ShellCorona::applicationConfig()
{
    return KSharedConfig::openConfig();
}

void ShellCorona::requestApplicationConfigSync()
{
    d->appConfigSyncTimer.start();
}

void ShellCorona::loadDefaultLayout()
{
    QString script = package().filePath("defaultlayout");
    qDebug() << "This is the default layout we are using:";

    QFile file(script);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        QString code = file.readAll();
        qDebug() << "evaluating startup script:" << script;
        d->scriptEngine->evaluateScript(code);
    }
}

void ShellCorona::processUpdateScripts()
{
    foreach (const QString &script, WorkspaceScripting::ScriptEngine::pendingUpdateScripts(this)) {
        d->scriptEngine->evaluateScript(script);
    }
}

Activity* ShellCorona::activity(const QString &id)
{
    if (!d->activities.contains(id)) {
        //the add signal comes late sometimes
        activityAdded(id);
    }
    return d->activities.value(id);
}

KActivities::Controller *ShellCorona::activityController()
{
    return d->activityController;
}

void ShellCorona::checkScreens(bool signalWhenExists)
{

    checkViews();

    // quick sanity check to ensure we have containments for each screen
    int num = numScreens();
    for (int i = 0; i < num; ++i) {
        checkScreen(i, signalWhenExists);
    }
}

void ShellCorona::checkScreen(int screen, bool signalWhenExists)
{
    // signalWhenExists is there to allow PlasmaApp to know when to create views
    // it does this only on containment addition, but in the case of a screen being
    // added and the containment already existing for that screen, no signal is emitted
    // and so PlasmaApp does not know that it needs to create a view for it. to avoid
    // taking care of that case in PlasmaApp (which would duplicate some of the code below,
    // ShellCorona will, when signalWhenExists is true, emit a containmentAdded signal
    // even if the containment actually existed prior to this method being called.
    //
    //note: hte signal actually triggers view creation only for panels, atm.
    //desktop views are created in response to containment's screenChanged signal instead, which is
    //buggy (sometimes the containment thinks it's already on the screen, so no view is created)

    //TODO: restore activities
    Activity *currentActivity = activity(d->activityController->currentActivity());
    //ensure the desktop(s) have a containment and view
    checkDesktop(currentActivity, signalWhenExists, screen);


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

void ShellCorona::checkDesktop(Activity *activity, bool signalWhenExists, int screen)
{
    Plasma::Containment *c = activity->containmentForScreen(screen);

    //TODO: remove following when activities are restored
    if (!c) {
        c = createContainment(d->desktopDefaultsConfig.readEntry("Containment", "org.kde.desktopcontainment"));
    }

    if (!c) {
        return;
    }

    c->setScreen(screen);
    if (screen >= 0 || d->views.count() >= screen + 1) {
        d->views[screen]->setContainment(c);
    } else {
        qWarning() << "Invalid screen";
    }
    c->flushPendingConstraintsEvents();
    requestApplicationConfigSync();

    if (signalWhenExists) {
        emit containmentAdded(c);
    }
}

int ShellCorona::numScreens() const
{
    return d->desktopWidget->screenCount();
}

QRect ShellCorona::screenGeometry(int id) const
{
    return d->desktopWidget->screenGeometry(id);
}

QRegion ShellCorona::availableScreenRegion(int id) const
{
    return d->desktopWidget->availableGeometry(id);
}

QRect ShellCorona::availableScreenRect(int id) const
{
    return d->desktopWidget->availableGeometry(id);
}

PanelView *ShellCorona::panelView(Plasma::Containment *containment) const
{
    return d->panelViews.value(containment);
}


///// SLOTS

void ShellCorona::screenCountChanged(int newCount)
{
    qDebug() << "New screen count" << newCount;
    checkViews();
}

void ShellCorona::screenResized(int screen)
{
    qDebug() << "Screen resized" << screen;
}

void ShellCorona::workAreaResized(int screen)
{
    qDebug() << "Work area resized" << screen;
}

void ShellCorona::checkViews()
{
    if (d->shell.isEmpty()) {
        return;
    }

    if (d->views.count() == d->desktopWidget->screenCount()) {
        return;
    } else if (d->views.count() < d->desktopWidget->screenCount()) {
        for (int i = d->views.count(); i < d->desktopWidget->screenCount(); ++i) {

            DesktopView *view = new DesktopView(this);
            QSurfaceFormat format;
            view->show();

            d->views << view;
        }
    } else {
        for (int i = d->desktopWidget->screenCount(); i < d->views.count(); ++i) {
            DesktopView *view = d->views.last();
            view->deleteLater();
            d->views.pop_back();
        }
    }

    //check every containment is in proper view
    for (int i = 0; i < d->desktopWidget->screenCount(); ++i) {
        qDebug() << "TODO: Implement loading containments into the views";
    }
}

void ShellCorona::checkLoadingDesktopsComplete()
{
    Plasma::Containment *c = qobject_cast<Plasma::Containment *>(sender());
    if (c) {
        disconnect(c, &Plasma::Containment::uiReadyChanged,
                   this, &ShellCorona::checkLoadingDesktopsComplete);
        d->loadingDesktops.remove(c);
    }

    if (d->loadingDesktops.isEmpty()) {
        d->waitingPanelsTimer->start();
    } else {
        d->waitingPanelsTimer->stop();
    }
}

void ShellCorona::createWaitingPanels()
{
    foreach (Plasma::Containment *cont, d->waitingPanels) {
        d->panelViews[cont] = new PanelView(this);
        d->panelViews[cont]->setContainment(cont);
    }
    d->waitingPanels.clear();
}

void ShellCorona::updateScreenOwner(int wasScreen, int isScreen, Plasma::Containment *containment)
{
    qDebug() << "Was screen" << wasScreen << "Is screen" << isScreen <<"Containment" << containment << containment->title();

    if (containment->formFactor() == Plasma::Types::Horizontal ||
        containment->formFactor() == Plasma::Types::Vertical) {

        if (isScreen >= 0) {
            if (!d->waitingPanels.contains(containment)) {
                d->waitingPanels << containment;
            }
        } else {
            if (d->panelViews.contains(containment)) {
                d->panelViews[containment]->setContainment(0);
                d->panelViews[containment]->deleteLater();
                d->panelViews.remove(containment);
                d->waitingPanels.removeAll(containment);
            }
        }
        checkLoadingDesktopsComplete();

    //Desktop view
    } else {

        if (containment->isUiReady()) {
            d->loadingDesktops.remove(containment);
            checkLoadingDesktopsComplete();
        } else {
            d->loadingDesktops.insert(containment);
            connect(containment, &Plasma::Containment::uiReadyChanged,
                    this, &ShellCorona::checkLoadingDesktopsComplete);
        }

        if (isScreen < 0 || d->views.count() < isScreen + 1) {
            qWarning() << "Invalid screen";
            return;
        }

        d->views[isScreen]->setContainment(containment);
    }
}

void ShellCorona::handleContainmentAdded(Plasma::Containment* c)
{
    connect(c, &Plasma::Containment::showAddWidgetsInterface,
            this, &ShellCorona::showWidgetExplorer);
    connect(c, &QObject::destroyed, [=] (QObject *o) {
        d->loadingDesktops.remove(static_cast<Plasma::Containment *>(o));
    });
}

void ShellCorona::showWidgetExplorer()
{
    QPoint cursorPos = QCursor::pos();
    foreach (DesktopView *view, d->views) {
        if (view->screen()->geometry().contains(cursorPos)) {
            if (!d->widgetExplorer) {
                QString expqml = package().filePath("widgetexplorer");
                qDebug() << "Script to load for WidgetExplorer: " << expqml;
                d->widgetExplorer = new WidgetExplorer();
                d->widgetExplorer.data()->setSource(QUrl::fromLocalFile(expqml));
            }
            Plasma::Containment *c = 0;
            c = dynamic_cast<Plasma::Containment*>(sender());
            if (c) {
                qDebug() << "Found containment.";
                d->widgetExplorer.data()->setContainment(c);
            } else {
                // FIXME: try harder to find a suitable containment?
                qWarning() << "containment not set, don't know where to add the applet.";
            }
            //The view QML has to provide something to display the activity explorer
            view->rootObject()->metaObject()->invokeMethod(view->rootObject(), "toggleWidgetExplorer", Q_ARG(QVariant, QVariant::fromValue(d->widgetExplorer.data())));
            return;
        }
    }


}

void ShellCorona::toggleActivityManager()
{
    QPoint cursorPos = QCursor::pos();
    foreach (DesktopView *view, d->views) {
        if (view->screen()->geometry().contains(cursorPos)) {
            //The view QML has to provide something to display the activity explorer
            view->rootObject()->metaObject()->invokeMethod(view->rootObject(), "toggleActivityManager");
            return;
        }
    }
}

void ShellCorona::syncAppConfig()
{
    qDebug() << "Syncing plasma-shellrc config";
    applicationConfig()->sync();
}

void ShellCorona::setDashboardShown(bool show)
{
    qDebug() << "TODO: Toggling dashboard view";

    QAction *dashboardAction = actions()->action("show dashboard");

    if (dashboardAction) {
        dashboardAction->setText(show ? i18n("Hide Dashboard") : i18n("Show Dashboard"));
    }
    foreach (DesktopView *view, d->views) {
        view->setDashboardShown(show);
    }
}

void ShellCorona::checkActivities()
{
    qDebug() << "containments to start with" << containments().count();

    KActivities::Consumer::ServiceStatus status = d->activityController->serviceStatus();
    //qDebug() << "$%$%$#%$%$%Status:" << status;
    if (status == KActivities::Consumer::NotRunning) {
        //panic and give up - better than causing a mess
        qDebug() << "No ActivityManager? Help, I've fallen and I can't get up!";
        return;
    }

    QStringList existingActivities = d->activityConsumer->activities();
    foreach (const QString &id, existingActivities) {
        activityAdded(id);
    }

    QStringList newActivities;
    QString newCurrentActivity;
    //migration checks:
    //-containments with an invalid id are deleted.
    //-containments that claim they were on a screen are kept together, and are preferred if we
    //need to initialize the current activity.
    //-containments that don't know where they were or who they were with just get made into their
    //own activity.
    foreach (Plasma::Containment *cont, containments()) {
        if ((cont->containmentType() == Plasma::Types::DesktopContainment ||
             cont->containmentType() == Plasma::Types::CustomContainment)) {
            const QString oldId = cont->activity();
            if (!oldId.isEmpty()) {
                if (existingActivities.contains(oldId)) {
                    continue; //it's already claimed
                }
                qDebug() << "invalid id" << oldId;
                //byebye
                cont->destroy();
                continue;
            }

            if (cont->screen() > -1 && !newCurrentActivity.isEmpty()) {
                //it belongs on the new current activity
                cont->setActivity(newCurrentActivity);
                continue;
            }

            /*//discourage blank names
            if (context->currentActivity().isEmpty()) {
                context->setCurrentActivity(i18nc("Default name for a new activity", "New Activity"));
            }*/

            //create a new activity for the containment
            const QString id = d->activityController->addActivity(cont->activity());
            cont->setActivity(id);
            newActivities << id;
            if (cont->screen() > -1) {
                newCurrentActivity = id;
            }
            qDebug() << "migrated" << cont->id() << cont->activity();
        }
    }

    qDebug() << "migrated?" << !newActivities.isEmpty() << containments().count();
    if (!newActivities.isEmpty()) {
        requestConfigSync();
    }

    //init the newbies
    foreach (const QString &id, newActivities) {
        activityAdded(id);
    }

    //ensure the current activity is initialized
    if (d->activityController->currentActivity().isEmpty()) {
        qDebug() << "guessing at current activity";
        if (existingActivities.isEmpty()) {
            if (newCurrentActivity.isEmpty()) {
                if (newActivities.isEmpty()) {
                    qDebug() << "no activities!?! Bad activitymanager, no cookie!";
                    QString id = d->activityController->addActivity(i18nc("Default name for a new activity", "New Activity"));
                    activityAdded(id);
                    d->activityController->setCurrentActivity(id);
                    qDebug() << "created emergency activity" << id;
                } else {
                    d->activityController->setCurrentActivity(newActivities.first());
                }
            } else {
                d->activityController->setCurrentActivity(newCurrentActivity);
            }
        } else {
            d->activityController->setCurrentActivity(existingActivities.first());
        }
    }
}

void ShellCorona::currentActivityChanged(const QString &newActivity)
{
    Activity *act = activity(newActivity);
    qDebug() << "Activity changed:" << newActivity << act;

    if (act) {
        act->ensureActive();
    }
}

void ShellCorona::activityAdded(const QString &id)
{
    //TODO more sanity checks
    if (d->activities.contains(id)) {
        qDebug() << "you're late." << id;
        return;
    }

    Activity *a = new Activity(id, this);
    if (a->isCurrent()) {
        a->ensureActive();
    }
    d->activities.insert(id, a);
}

void ShellCorona::activityRemoved(const QString &id)
{
    Activity *a = d->activities.take(id);
    a->deleteLater();
}

void ShellCorona::printScriptError(const QString &error)
{
    qWarning() << error;
}

void ShellCorona::printScriptMessage(const QString &message)
{
    qDebug() << message;
}

// Desktop corona handler


#include "moc_shellcorona.cpp"

