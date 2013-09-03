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

#include <KLocalizedString>
#include <Plasma/Package>
#include <Plasma/PluginLoader>

#include "containmentconfigview.h"
#include "panelview.h"
#include "view.h"
#include "scripting/desktopscriptengine.h"
#include "widgetexplorer/widgetexplorerview.h"
#include "configview.h"

class ShellCorona::Private {
public:
    Private()
        : desktopWidget(QApplication::desktop()),
          widgetExplorerView(nullptr)
    {
        appConfigSyncTimer.setSingleShot(true);
        // constant controlling how long between requesting a configuration sync
        // and one happening should occur. currently 10 seconds
        appConfigSyncTimer.setInterval(10000);
    }

    QString shell;
    QDesktopWidget * desktopWidget;
    QList <View *> views;
    WidgetExplorerView * widgetExplorerView;
    QHash <Plasma::Containment *, PanelView *> panelViews;
    KConfigGroup desktopDefaultsConfig;
    WorkspaceScripting::DesktopScriptEngine * scriptEngine;
    QList<Plasma::Containment *> waitingPanels;
    QSet<Plasma::Containment *> loadingDesktops;

    QTimer appConfigSyncTimer;
};

WorkspaceScripting::DesktopScriptEngine * ShellCorona::scriptEngine() const
{
    return d->scriptEngine;
}


ShellCorona::ShellCorona(QObject *parent)
    : Plasma::Corona(parent), d(new Private())
{
    d->desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package().filePath("defaults")), "Desktop");

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

    //QTimer::singleShot(600, this, SLOT(showWidgetExplorer())); // just for easier debugging
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
    //Activity *currentActivity = activity(d->activityController->currentActivity());
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

void ShellCorona::checkDesktop(/*Activity *activity,*/ bool signalWhenExists, int screen)
{
    Plasma::Containment *c = /*activity->*/containmentForScreen(screen);

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

            View *view = new View(this);
            QSurfaceFormat format;
            view->show();

            d->views << view;
        }
    } else {
        for (int i = d->desktopWidget->screenCount(); i < d->views.count(); ++i) {
            View *view = d->views.last();
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
        foreach (Plasma::Containment *cont, d->waitingPanels) {
            d->panelViews[cont] = new PanelView(this);
            d->panelViews[cont]->setContainment(cont);
        }
        d->waitingPanels.clear();
    }
}

void ShellCorona::updateScreenOwner(int wasScreen, int isScreen, Plasma::Containment *containment)
{
    qDebug() << "Was screen" << wasScreen << "Is screen" << isScreen <<"Containment" << containment << containment->title();

    if (containment->formFactor() == Plasma::Types::Horizontal ||
        containment->formFactor() == Plasma::Types::Vertical) {

        if (isScreen >= 0) {
            d->waitingPanels << containment;
        } else {
            if (d->panelViews.contains(containment)) {
                d->panelViews[containment]->setContainment(0);
                d->panelViews[containment]->deleteLater();
                d->panelViews.remove(containment);
            }
        }
        checkLoadingDesktopsComplete();

    //Desktop view
    } else {

        if (containment->isUiReady()) {
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
}

void ShellCorona::showWidgetExplorer()
{
    if (!d->widgetExplorerView) {

        QString expqml = package().filePath("widgetexplorer");
        qDebug() << "Script to load for WidgetExplorer: " << expqml;
        d->widgetExplorerView = new WidgetExplorerView(expqml);
        d->widgetExplorerView->init();
    }
    Plasma::Containment *c = 0;
    c = dynamic_cast<Plasma::Containment*>(sender());
    if (c) {
        qDebug() << "Found containment.";
        d->widgetExplorerView->setContainment(c);
    } else {
        // FIXME: try harder to find a suitable containment?
        qWarning() << "containment not set, don't know where to add the applet.";
    }
    d->widgetExplorerView->show();
}

void ShellCorona::syncAppConfig()
{
    qDebug() << "Syncing plasma-shellrc config";
    applicationConfig()->sync();
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

