/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *   Copyright 2013 Ivan Cukic <ivan.cukic@kde.org>
 *   Copyright 2013 Marco Martin <mart@kde.org>
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
#include <QMenu>
#include <QDesktopWidget>
#include <QQmlContext>
#include <QTimer>
#include <QDBusConnection>

#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <Plasma/Package>
#include <Plasma/PluginLoader>
#include <kactivities/controller.h>
#include <kactivities/consumer.h>
#include <ksycoca.h>
#include <kservicetypetrader.h>
#include <KGlobalAccel>
#include <KAuthorized>
#include <KWindowSystem>

#include "config-ktexteditor.h" // HAVE_KTEXTEDITOR


#include "activity.h"
#include "desktopview.h"
#include "panelview.h"
#include "scripting/desktopscriptengine.h"
#include "widgetexplorer/widgetexplorer.h"
#include "configview.h"
#include "shellpluginloader.h"
#include "osd.h"
#if HAVE_KTEXTEDITOR
#include "interactiveconsole.h"
#endif

#include "plasmashelladaptor.h"



static const int s_configSyncDelay = 10000; // 10 seconds

class ShellCorona::Private {
public:
    Private(ShellCorona *corona)
        : q(corona),
          activityController(new KActivities::Controller(q)),
          activityConsumer(new KActivities::Consumer(q)),
          addPanelAction(nullptr),
          addPanelsMenu(nullptr)
    {
        appConfigSyncTimer.setSingleShot(true);
        appConfigSyncTimer.setInterval(s_configSyncDelay);
        connect(&appConfigSyncTimer, &QTimer::timeout, q, &ShellCorona::syncAppConfig);

        waitingPanelsTimer.setSingleShot(true);
        waitingPanelsTimer.setInterval(250);
        connect(&waitingPanelsTimer, &QTimer::timeout, q, &ShellCorona::createWaitingPanels);
    }

    ShellCorona *q;
    QString shell;
    QList<DesktopView *> views;
    KActivities::Controller *activityController;
    KActivities::Consumer *activityConsumer;
    QHash<const Plasma::Containment *, PanelView *> panelViews;
    KConfigGroup desktopDefaultsConfig;
    WorkspaceScripting::DesktopScriptEngine *scriptEngine;
    QList<Plasma::Containment *> waitingPanels;
    QHash<QString, Activity *> activities;
    QHash<QString, QHash<int, Plasma::Containment *> > desktopContainments;
    QAction *addPanelAction;
    QMenu *addPanelsMenu;
    Plasma::Package lookNFeelPackage;
#if HAVE_KTEXTEDITOR
    QWeakPointer<InteractiveConsole> console;
#endif

    QTimer waitingPanelsTimer;
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

    new PlasmaShellAdaptor(this);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/PlasmaShell"), this);


    // Look for theme config in plasmarc, if it isn't configured, take the theme from the
    // LookAndFeel package, if either is set, change the default theme

    const QString themeGroupKey = QStringLiteral("Theme");
    const QString themeNameKey = QStringLiteral("name");

    QString themeName;

    KConfigGroup plasmarc(KSharedConfig::openConfig("plasmarc"), themeGroupKey);
    themeName = plasmarc.readEntry(themeNameKey, themeName);

    if (themeName.isEmpty()) {
        const KConfigGroup lnfCfg = KConfigGroup(KSharedConfig::openConfig(
                                                lookAndFeelPackage().filePath("defaults")),
                                                themeGroupKey
                                           );
        themeName = lnfCfg.readEntry(themeNameKey, themeName);
    }

    if (!themeName.isEmpty()) {
        Plasma::Theme *t = new Plasma::Theme(this);
        t->setThemeName(themeName);
    }

    connect(this, &ShellCorona::containmentAdded,
            this, &ShellCorona::handleContainmentAdded);

    d->scriptEngine = new WorkspaceScripting::DesktopScriptEngine(this, true);

    connect(d->scriptEngine, &WorkspaceScripting::ScriptEngine::printError,
            [=](const QString &msg) {
                qWarning() << msg;
            });
    connect(d->scriptEngine, &WorkspaceScripting::ScriptEngine::print,
            [=](const QString &msg) {
                qDebug() << msg;
            });

    QAction *dashboardAction = actions()->add<QAction>("show dashboard");
    QObject::connect(dashboardAction, &QAction::triggered,
                     this, &ShellCorona::setDashboardShown);
    dashboardAction->setText(i18n("Show Dashboard"));

    dashboardAction->setAutoRepeat(true);
    dashboardAction->setCheckable(true);
    dashboardAction->setIcon(QIcon::fromTheme("dashboard-show"));
    dashboardAction->setData(Plasma::Types::ControlAction);
    KGlobalAccel::self()->setDefaultShortcut(dashboardAction, QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::Key_F12));
    KGlobalAccel::self()->setShortcut(dashboardAction, QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::Key_F12));


    checkAddPanelAction();
    connect(KSycoca::self(), SIGNAL(databaseChanged(QStringList)), this, SLOT(checkAddPanelAction(QStringList)));


    //Activity stuff
    QAction *activityAction = actions()->addAction("manage activities");
    connect(activityAction, &QAction::triggered,
            this, &ShellCorona::toggleActivityManager);
    activityAction->setText(i18n("Activities..."));
    activityAction->setIcon(QIcon::fromTheme("preferences-activities"));
    activityAction->setData(Plasma::Types::ConfigureAction);
    activityAction->setShortcut(QKeySequence("alt+d, alt+a"));
    activityAction->setShortcutContext(Qt::ApplicationShortcut);

    connect(d->activityConsumer, SIGNAL(currentActivityChanged(QString)), this, SLOT(currentActivityChanged(QString)));
    connect(d->activityConsumer, SIGNAL(activityAdded(QString)), this, SLOT(activityAdded(QString)));
    connect(d->activityConsumer, SIGNAL(activityRemoved(QString)), this, SLOT(activityRemoved(QString)));

    new Osd(this);
}

ShellCorona::~ShellCorona()
{
    qDeleteAll(d->views);
}

void ShellCorona::setShell(const QString &shell)
{
    if (d->shell == shell) {
        return;
    }

    unload();

    d->shell = shell;
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Shell");
    package.setPath(shell);
    setPackage(package);

    if (d->activityConsumer->serviceStatus() == KActivities::Consumer::Unknown) {
        connect(d->activityConsumer, SIGNAL(serviceStatusChanged(Consumer::ServiceStatus)), SLOT(load()), Qt::UniqueConnection);
    } else {
        load();
    }
}

QString ShellCorona::shell() const
{
    return d->shell;
}

void ShellCorona::load()
{
    if (d->shell.isEmpty() ||
        d->activityConsumer->serviceStatus() == KActivities::Consumer::Unknown) {
        return;
    }

    disconnect(d->activityConsumer, SIGNAL(serviceStatusChanged(Consumer::ServiceStatus)), this, SLOT(load()));

    loadLayout("plasma-" + d->shell + "-appletsrc");

    checkActivities();
    if (containments().isEmpty()) {
        loadDefaultLayout();
        foreach(Plasma::Containment* containment, containments()) {
            containment->setActivity(d->activityConsumer->currentActivity());
        }
    }

    processUpdateScripts();
    foreach(Plasma::Containment* containment, containments()) {
        if (containment->formFactor() == Plasma::Types::Horizontal ||
            containment->formFactor() == Plasma::Types::Vertical) {
            if (!d->waitingPanels.contains(containment)) {
                d->waitingPanels << containment;
            }
        } else {
            //FIXME ideally fix this, or at least document the crap out of it
            int screen = containment->lastScreen();
            if (screen < 0) {
                screen = d->desktopContainments[containment->activity()].count();
            }
            insertContainment(containment->activity(), screen, containment);
        }
    }

    for (QScreen *screen : QGuiApplication::screens()) {
        screenAdded(screen);
    }
    connect(qApp, &QGuiApplication::screenAdded,
            this, &ShellCorona::screenAdded);

    if (!d->waitingPanels.isEmpty()) {
        d->waitingPanelsTimer.start();
    }
}

void ShellCorona::unload()
{
    if (d->shell.isEmpty()) {
        return;
    }

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
    const QString script = package().filePath("defaultlayout");
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

KActivities::Controller *ShellCorona::activityController()
{
    return d->activityController;
}

int ShellCorona::numScreens() const
{
    return QApplication::desktop()->screenCount();
}

QRect ShellCorona::screenGeometry(int id) const
{
    return QApplication::desktop()->screenGeometry(id);
}

QRegion ShellCorona::availableScreenRegion(int id) const
{
    return QApplication::desktop()->availableGeometry(id);
}

QRect ShellCorona::availableScreenRect(int id) const
{
    //return QApplication::desktop()->availableGeometry(id);
    //FIXME: revert back to this^ after https://codereview.qt-project.org/#change,80606 has been merged
    //       and released (and we depend on it)
    return KWindowSystem::workArea(id).intersect(QApplication::desktop()->availableGeometry(id));
}

PanelView *ShellCorona::panelView(Plasma::Containment *containment) const
{
    return d->panelViews.value(containment);
}


///// SLOTS

void ShellCorona::screenAdded(QScreen *screen)
{
    DesktopView *view = new DesktopView(this, screen);
    const QString currentActivity = d->activityController->currentActivity();

    if (!d->views.isEmpty() && screen == QGuiApplication::primaryScreen()) {
        DesktopView* oldPrimaryView = d->views.first();
        QScreen* oldPrimaryScreen = oldPrimaryView->screen();

        //move any panels that were preivously on the old primary screen to the new primary screen
        foreach (PanelView *panelView, d->panelViews) {
            if (oldPrimaryScreen==panelView->screen())
                panelView->setScreen(screen);
        }

        Plasma::Containment* primaryContainment = oldPrimaryView->containment();
        oldPrimaryView->setContainment(0);
        view->setContainment(primaryContainment);

        d->views.prepend(view);
        view = oldPrimaryView;
    } else {
        d->views.append(view);
    }

    int screenNum = d->views.count()-1;
    Plasma::Containment *containment = d->desktopContainments[currentActivity][screenNum];
    if (!containment) {
        containment = createContainmentForActivity(currentActivity, screenNum);
    }

    QAction *removeAction = containment->actions()->action("remove");
    if (removeAction) {
        removeAction->deleteLater();
    }

    view->setContainment(containment);

    connect(screen, SIGNAL(destroyed(QObject*)), SLOT(screenRemoved(QObject*)));
    view->show();

    emit availableScreenRectChanged();
    emit availableScreenRegionChanged();
}


Plasma::Containment* ShellCorona::createContainmentForActivity(const QString& activity, int screenNum)
{
    if (d->desktopContainments.contains(activity) &&
        d->desktopContainments[activity].contains(screenNum) &&
        d->desktopContainments.value(activity).value(screenNum)) {
        return d->desktopContainments[activity][screenNum];
    }

    QString plugin = "org.kde.desktopcontainment";
    if (d->activities.contains(activity)) {
      //  plugin = d->activities.value(activity)->defaultPlugin();
    }

    Plasma::Containment* containment = createContainment(d->desktopDefaultsConfig.readEntry("Containment", plugin));
    containment->setActivity(activity);
    insertContainment(activity, screenNum, containment);

    return containment;
}

void ShellCorona::screenRemoved(QObject *screen)
{
    //desktop containments
    for (auto i = d->views.begin(); i != d->views.end(); i++) {
        if ((*i)->screen() == screen) {
            d->views.erase(i);
            (*i)->containment()->reactToScreenChange();
            (*i)->deleteLater();
            break;
        }
    }

    //move all panels on a deleted screen to the primary screen
    //FIXME: this will break when a second screen is added again
    //as in plasma1, panel should be hidden, panelView deleted.
    //possibly similar to exportLayout/importLayout of Activities
    foreach (PanelView *view, d->panelViews) {
        view->setScreen(QGuiApplication::primaryScreen());
    }

    emit availableScreenRectChanged();
    emit availableScreenRegionChanged();

}

void ShellCorona::createWaitingPanels()
{
    foreach (Plasma::Containment *cont, d->waitingPanels) {
        d->panelViews[cont] = new PanelView(this);

        //keep screen suggestions within bounds of screens we actually have
        int screen = qBound(0, cont->lastScreen(), QGuiApplication::screens().size() -1);

        d->panelViews[cont]->setScreen(QGuiApplication::screens()[screen]);
        d->panelViews[cont]->setContainment(cont);
        connect(cont, &PanelView::destroyed,
                [=](QObject *obj) {
                    d->panelViews.remove(cont);
                });
    }
    d->waitingPanels.clear();
}

void ShellCorona::handleContainmentAdded(Plasma::Containment* c)
{
    connect(c, &Plasma::Containment::showAddWidgetsInterface,
            this, &ShellCorona::toggleWidgetExplorer);
}

void ShellCorona::toggleWidgetExplorer()
{
    const QPoint cursorPos = QCursor::pos();
    foreach (DesktopView *view, d->views) {
        if (view->screen()->geometry().contains(cursorPos)) {
            //The view QML has to provide something to display the activity explorer
            view->rootObject()->metaObject()->invokeMethod(view->rootObject(), "toggleWidgetExplorer", Q_ARG(QVariant, QVariant::fromValue(sender())));
            return;
        }
    }
}

void ShellCorona::toggleActivityManager()
{
    const QPoint cursorPos = QCursor::pos();
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

void ShellCorona::toggleDashboard()
{
    foreach (DesktopView *view, d->views) {
        view->setDashboardShown(!view->isDashboardShown());
    }
}

void ShellCorona::showInteractiveConsole()
{
    if (KSharedConfig::openConfig()->isImmutable() || !KAuthorized::authorize("plasma-desktop/scripting_console")) {
        return;
    }

#if HAVE_KTEXTEDITOR
    InteractiveConsole *console = d->console.data();
    if (!console) {
        d->console = console = new InteractiveConsole(this);
    }
    d->console.data()->setMode(InteractiveConsole::PlasmaConsole);

    KWindowSystem::setOnDesktop(console->winId(), KWindowSystem::currentDesktop());
    console->show();
    console->raise();
    KWindowSystem::forceActiveWindow(console->winId());
#endif
}

void ShellCorona::loadScriptInInteractiveConsole(const QString &script)
{
#if HAVE_KTEXTEDITOR
    showInteractiveConsole();
    if (d->console) {
        d->console.data()->loadScript(script);
    }
#endif
}

void ShellCorona::checkActivities()
{
    KActivities::Consumer::ServiceStatus status = d->activityController->serviceStatus();
    //qDebug() << "$%$%$#%$%$%Status:" << status;
    if (status != KActivities::Consumer::Running) {
        //panic and give up - better than causing a mess
        qDebug() << "ShellCorona::checkActivities is called whilst activity daemon is still connecting";
        return;
    }

    QStringList existingActivities = d->activityConsumer->activities();
    foreach (const QString &id, existingActivities) {
        activityAdded(id);
    }

    // Checking whether the result we got is valid. Just in case.
    Q_ASSERT_X(!existingActivities.isEmpty(), "isEmpty", "There are no activities, and the service is running");
    Q_ASSERT_X(existingActivities[0] != QStringLiteral("00000000-0000-0000-0000-000000000000"),
            "null uuid", "There is a nulluuid activity present");

    // Killing the unassigned containments
    foreach (Plasma::Containment * cont, containments()) {
        if ((cont->containmentType() == Plasma::Types::DesktopContainment ||
             cont->containmentType() == Plasma::Types::CustomContainment) &&
            !existingActivities.contains(cont->activity())) {
            cont->destroy();
        }
    }
}

void ShellCorona::currentActivityChanged(const QString &newActivity)
{
    qDebug() << "Activity changed:" << newActivity;

    for (int i = 0; i < d->views.count(); ++i) {
        Plasma::Containment* c = d->desktopContainments[newActivity][i];
        if (!c) {
            c = createContainmentForActivity(newActivity, i);
        }
        QAction *removeAction = c->actions()->action("remove");
        if (removeAction) {
            removeAction->deleteLater();
        }
        d->views[i]->setContainment(c);
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
    d->activities.insert(id, a);
    createContainmentForActivity(id, -1);
}

void ShellCorona::activityRemoved(const QString &id)
{
    Activity *a = d->activities.take(id);
    a->deleteLater();
}

Activity *ShellCorona::activity(const QString &id)
{
    return d->activities.value(id);
}

void ShellCorona::insertActivity(const QString &id, Activity *activity)
{
    d->activities.insert(id, activity);
    createContainmentForActivity(id, -1);
}

void ShellCorona::checkAddPanelAction(const QStringList &sycocaChanges)
{
    if (!sycocaChanges.isEmpty() && !sycocaChanges.contains("services")) {
        return;
    }

    delete d->addPanelAction;
    d->addPanelAction = 0;

    delete d->addPanelsMenu;
    d->addPanelsMenu = 0;

    KPluginInfo::List panelContainmentPlugins = Plasma::PluginLoader::listContainmentsOfType("Panel");
    const QString constraint = QString("[X-Plasma-Shell] == '%1' and 'panel' ~in [X-Plasma-ContainmentCategories]")
                                      .arg(qApp->applicationName());
    KService::List templates = KServiceTypeTrader::self()->query("Plasma/LayoutTemplate", constraint);

    if (panelContainmentPlugins.count() + templates.count() == 1) {
        d->addPanelAction = new QAction(i18n("Add Panel"), this);
        d->addPanelAction->setData(Plasma::Types::AddAction);
        connect(d->addPanelAction, SIGNAL(triggered(bool)), this, SLOT(addPanel()));
    } else if (!panelContainmentPlugins.isEmpty()) {
        d->addPanelsMenu = new QMenu;
        d->addPanelAction = d->addPanelsMenu->menuAction();
        d->addPanelAction->setText(i18n("Add Panel"));
        d->addPanelAction->setData(Plasma::Types::AddAction);
        qDebug() << "populateAddPanelsMenu" << panelContainmentPlugins.count();
        connect(d->addPanelsMenu, SIGNAL(aboutToShow()), this, SLOT(populateAddPanelsMenu()));
        connect(d->addPanelsMenu, SIGNAL(triggered(QAction*)), this, SLOT(addPanel(QAction*)));
    }

    if (d->addPanelAction) {
        d->addPanelAction->setIcon(QIcon::fromTheme("list-add"));
        actions()->addAction("add panel", d->addPanelAction);
    }
}

void ShellCorona::addPanel()
{
    KPluginInfo::List panelPlugins = Plasma::PluginLoader::listContainmentsOfType("Panel");

    if (!panelPlugins.isEmpty()) {
        addPanel(panelPlugins.first().pluginName());
    }
}

void ShellCorona::addPanel(QAction *action)
{
    const QString plugin = action->data().toString();
    if (plugin.startsWith("plasma-desktop-template:")) {
        d->scriptEngine->evaluateScript(plugin.right(plugin.length() - qstrlen("plasma-desktop-template:")));
    } else if (!plugin.isEmpty()) {
        addPanel(plugin);
    }
}

void ShellCorona::addPanel(const QString &plugin)
{
    Plasma::Containment *panel = createContainment(plugin);
    if (!panel) {
        return;
    }

    QList<Plasma::Types::Location> availableLocations;
    availableLocations << Plasma::Types::LeftEdge << Plasma::Types::TopEdge << Plasma::Types::RightEdge << Plasma::Types::BottomEdge;

    foreach (const Plasma::Containment *cont, d->panelViews.keys()) {
        availableLocations.removeAll(cont->location());
    }

    Plasma::Types::Location loc;
    if (availableLocations.isEmpty()) {
        loc = Plasma::Types::TopEdge;
    } else {
        loc = availableLocations.first();
    }

    panel->setLocation(loc);
    switch (loc) {
    case Plasma::Types::LeftEdge:
    case Plasma::Types::RightEdge:
        panel->setFormFactor(Plasma::Types::Vertical);
        break;
    default:
        panel->setFormFactor(Plasma::Types::Horizontal);
        break;
    }

    d->waitingPanels << panel;
    d->waitingPanelsTimer.start();
}

int ShellCorona::screenForContainment(const Plasma::Containment *containment) const
{
    QScreen *screen = nullptr;
    for (int i = 0; i < d->views.size(); i++) {
        if (d->views[i]->containment() == containment) {
            screen = d->views[i]->screen();
        }
    }

    if (!screen) {
        PanelView *view = d->panelViews[containment];
        if (view) {
            screen = view->screen();
        }
    }

    return screen ? qApp->screens().indexOf(screen) : -1;
}

void ShellCorona::activityOpened()
{
    Activity *activity = qobject_cast<Activity *>(sender());
    if (activity) {
        QList<Plasma::Containment*> cs = importLayout(activity->config());
        for (Plasma::Containment *containment : cs) {
            insertContainment(activity->name(), containment->lastScreen(), containment);
        }
    }
}

void ShellCorona::activityClosed()
{
    Activity *activity = qobject_cast<Activity *>(sender());
    if (activity) {
        KConfigGroup cg = activity->config();
        exportLayout(cg, d->desktopContainments.value(activity->name()).values());
    }
}

void ShellCorona::activityRemoved()
{
    //when an activity is removed delete all associated desktop containments
    Activity *activity = qobject_cast<Activity *>(sender());
    if (activity) {
        QHash< int, Plasma::Containment* > containmentHash = d->desktopContainments.take(activity->name());
        for (auto a : containmentHash) {
            a->destroy();
        }
    }
}

void ShellCorona::insertContainment(const QString &activity, int screenNum, Plasma::Containment* containment)
{
    d->desktopContainments[activity][screenNum] = containment;

    //when a containment gets deleted update our map of containments
    connect(containment, &QObject::destroyed, [=](QObject *obj) {
        // when QObject::destroyed arrives, ~Plasma::Containment has run,
        // members of Containment are not accessible anymore,
        // so keep ugly bookeeping by hand
        auto containment = static_cast<Plasma::Containment*>(obj);
        for (auto a : d->desktopContainments) {
            QMutableHashIterator<int, Plasma::Containment *> it(a);
            while (it.hasNext()) {
                it.next();
                if (it.value() == containment) {
                    it.remove();
                    return;
                }
            }
        }
    });
}

Plasma::Package ShellCorona::lookAndFeelPackage() const
{
    if (!d->lookNFeelPackage.isValid()) {
        d->lookNFeelPackage = ShellPluginLoader::self()->loadPackage("Plasma/LookAndFeel");
        //TODO: make loading from config once we have some UI for setting the package
        d->lookNFeelPackage.setPath("org.kde.lookandfeel");
    }

    return d->lookNFeelPackage;
}


// Desktop corona handler


#include "moc_shellcorona.cpp"

