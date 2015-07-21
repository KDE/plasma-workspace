/*****************************************************************

Copyright 2008 Christian Mollekopf <chrigi_1@hotmail.com>
Copyright (C) 2011 Craig Drummond <craig@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

// Own
#include "taskitem.h"
#include "launcheritem.h"

#include <KActivities/Info>
#include <KConfig>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KFileItem>
#include <KService>
#include <KServiceTypeTrader>
#include <processcore/processes.h>
#include <processcore/process.h>

#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QDir>
#include <QDebug>

#include "groupmanager.h"

namespace TaskManager
{


class TaskItemPrivate
{
public:
    TaskItemPrivate(TaskItem *item)
        : q(item),
        announceIconChanges(false),
        checkedForLauncher(false) {
    }

    void filterChange(::TaskManager::TaskChanges change);
    TaskItem *q;
    QWeakPointer<Task> task;
    QWeakPointer<Startup> startupTask;
    QUrl launcherUrl;
    QIcon launcherIcon;
    bool announceIconChanges;
    bool checkedForLauncher;
    QString taskName;
};

void TaskItemPrivate::filterChange(::TaskManager::TaskChanges change)
{
    if (announceIconChanges || change != ::TaskManager::TaskChange::IconChanged) {
        emit q->changed(change);
    }
}

TaskItem::TaskItem(QObject *parent, Task *task)
    : AbstractGroupableItem(parent),
      d(new TaskItemPrivate(this))
{
    setTaskPointer(task);
}


TaskItem::TaskItem(QObject *parent, Startup *task)
    : AbstractGroupableItem(parent),
      d(new TaskItemPrivate(this))
{
    d->startupTask = task;
    connect(task, SIGNAL(changed(::TaskManager::TaskChanges)), this, SLOT(filterChange(::TaskManager::TaskChanges)));
    connect(task, SIGNAL(destroyed(QObject*)), this, SLOT(taskDestroyed())); //this item isn't useful anymore if the Task was closed
}

TaskItem::~TaskItem()
{
    emit destroyed(this);
    //qDebug();
    /*  if (parentGroup()) {
          parentGroup()->remove(this);
      }*/
    delete d;
}

void TaskItem::taskDestroyed()
{
    d->startupTask.clear();
    d->task.clear();
    // FIXME: due to a bug in Qt 4.x, the event loop reference count is incorrect
    // when going through x11EventFilter .. :/ so we have to singleShot the deleteLater
    QTimer::singleShot(0, this, SLOT(deleteLater()));
}

void TaskItem::setTaskPointer(Task *task)
{
    const bool differentTask = d->task.data() != task;

    if (d->startupTask) {
        disconnect(d->startupTask.data(), 0, this, 0);
        d->startupTask.clear();
    } else if (differentTask) {
        // if we aren't moving from startup -> task and the task pointer is changing on us
        // let's clear the launcher url
        d->launcherUrl.clear();
    }

    resetLauncherCheck();

    if (differentTask) {
        if (d->task) {
            disconnect(d->task.data(), 0, this, 0);
        }

        d->task = task;

        if (task) {
            connect(task, SIGNAL(changed(::TaskManager::TaskChanges)), this, SLOT(filterChange(::TaskManager::TaskChanges)));
            connect(task, SIGNAL(destroyed(QObject*)), this, SLOT(taskDestroyed()));
            emit gotTaskPointer();
        }
    }

    if (!d->task) {
        // FIXME: due to a bug in Qt 4.x, the event loop reference count is incorrect
        // when going through x11EventFilter .. :/ so we have to singleShot the deleteLater
        QTimer::singleShot(0, this, SLOT(deleteLater()));
    }
}

Task *TaskItem::task() const
{
    return d->task.data();
}

Startup *TaskItem::startup() const
{
    /*
    if (d->startupTask.isNull()) {
        qDebug() << "pointer is Null";
    }
    */
    return d->startupTask.data();
}

bool TaskItem::isStartupItem() const
{
    return d->startupTask;
}

WindowList TaskItem::winIds() const
{
    if (!d->task) {
        qDebug() << "no winId: probably startup task";
        return WindowList();
    }
    WindowList list;
    list << d->task.data()->window();
    return list;
}

QIcon TaskItem::icon() const
{
    if (d->task) {
        if (static_cast<GroupManager *>(parent())->alwaysUseLauncherIcons()) {
            if (d->launcherIcon.isNull()) {
                d->launcherIcon = launcherIconFromUrl(launcherUrl());
            }

            if (!d->launcherIcon.isNull()) {
                return d->launcherIcon;
            } else {
                return d->task.data()->icon();
            }
        } else {
            return d->task.data()->icon();
        }
    }

    if (d->startupTask) {
        return d->startupTask.data()->icon();
    }

    return QIcon();
}

bool TaskItem::announceIconChanges() const
{
    return d->announceIconChanges;
}

void TaskItem::setAnnounceIconChanges(bool announce)
{
    d->announceIconChanges = announce;
}

QString TaskItem::name() const
{
    if (d->task) {
        return d->task.data()->visibleName();
    }

    if (d->startupTask) {
        return d->startupTask.data()->text();
    }

    return QString();
}

QString TaskItem::taskName() const
{
    if (d->taskName.isEmpty()) {
        QUrl lUrl = launcherUrl();

        if (!lUrl.isEmpty() && lUrl.isLocalFile() && KDesktopFile::isDesktopFile(lUrl.toLocalFile())) {
            KDesktopFile f(lUrl.toLocalFile());

            if (f.tryExec()) {
                d->taskName = f.readName();
            }
        }
        if (d->taskName.isEmpty() && d->task) {
            d->taskName = d->task.data()->classClass().toLower();
        }
    }

    return d->taskName;
}

QStringList TaskItem::activities() const
{
    if (!task()) {
        return QStringList();
    }

    return task()->activities();
}

QStringList TaskItem::activityNames(bool includeCurrent) const
{
    if (!task()) {
        return QStringList();
    }

    QStringList names;

    QStringList activities = task()->activities();
    if (!includeCurrent) {
        activities.removeOne(TaskManager::self()->currentActivity());
    }

    Q_FOREACH(QString activity, activities) {
        KActivities::Info info(activity);
        if (info.state() != KActivities::Info::Invalid) {
            names << info.name();
        }
    }

    return names;
}

ItemType TaskItem::itemType() const
{
    return TaskItemType;
}

bool TaskItem::isGroupItem() const
{
    return false;
}

void TaskItem::setShaded(bool state)
{
    if (!d->task) {
        return;
    }
    d->task.data()->setShaded(state);
}

void TaskItem::toggleShaded()
{
    if (!d->task) {
        return;
    }
    d->task.data()->toggleShaded();
}

bool TaskItem::isShaded() const
{
    if (!d->task) {
        return false;
    }
    return d->task.data()->isShaded();
}

void TaskItem::toDesktop(int desk)
{
    if (!d->task) {
        return;
    }
    d->task.data()->toDesktop(desk);
}

bool TaskItem::isOnCurrentDesktop() const
{
    return d->task && d->task.data()->isOnCurrentDesktop();
}

bool TaskItem::isOnAllDesktops() const
{
    return d->task && d->task.data()->isOnAllDesktops();
}

int TaskItem::desktop() const
{
    if (!d->task) {
        return 0;
    }
    return d->task.data()->desktop();
}

void TaskItem::setMaximized(bool state)
{
    if (!d->task) {
        return;
    }
    d->task.data()->setMaximized(state);
}

void TaskItem::toggleMaximized()
{
    if (!d->task) {
        return;
    }
    d->task.data()->toggleMaximized();
}

bool TaskItem::isMaximized() const
{
    return d->task && d->task.data()->isMaximized();
}

void TaskItem::setMinimized(bool state)
{
    if (!d->task) {
        return;
    }
    d->task.data()->setIconified(state);
}

void TaskItem::toggleMinimized()
{
    if (!d->task) {
        return;
    }
    d->task.data()->toggleIconified();
}

bool TaskItem::isMinimized() const
{
    if (!d->task) {
        return false;
    }
    return d->task.data()->isMinimized();
}

void TaskItem::setFullScreen(bool state)
{
    if (!d->task) {
        return;
    }
    d->task.data()->setFullScreen(state);
}

void TaskItem::toggleFullScreen()
{
    if (!d->task) {
        return;
    }
    d->task.data()->toggleFullScreen();
}

bool TaskItem::isFullScreen() const
{
    if (!d->task) {
        return false;
    }
    return d->task.data()->isFullScreen();
}

void TaskItem::setKeptBelowOthers(bool state)
{
    if (!d->task) {
        return;
    }
    d->task.data()->setKeptBelowOthers(state);
}

void TaskItem::toggleKeptBelowOthers()
{
    if (!d->task) {
        return;
    }
    d->task.data()->toggleKeptBelowOthers();
}

bool TaskItem::isKeptBelowOthers() const
{
    if (!d->task) {
        return false;
    }
    return d->task.data()->isKeptBelowOthers();
}

void TaskItem::setAlwaysOnTop(bool state)
{
    if (!d->task) {
        return;
    }
    d->task.data()->setAlwaysOnTop(state);
}

void TaskItem::toggleAlwaysOnTop()
{
    if (!d->task) {
        return;
    }
    d->task.data()->toggleAlwaysOnTop();
}

bool TaskItem::isAlwaysOnTop() const
{
    if (!d->task) {
        return false;
    }
    return d->task.data()->isAlwaysOnTop();
}

bool TaskItem::isActionSupported(NET::Action action) const
{
    return d->task && d->task.data()->info().actionSupported(action);
}

void TaskItem::addMimeData(QMimeData *mimeData) const
{
    if (!d->task) {
        return;
    }

    d->task.data()->addMimeData(mimeData);
}

void TaskItem::setLauncherUrl(const QUrl &url)
{
    if (!d->launcherUrl.isEmpty()) {
        return;
    }

    d->launcherUrl = url;

    if (static_cast<GroupManager *>(parent())->alwaysUseLauncherIcons()) {
        d->launcherIcon = launcherIconFromUrl(url);
    }

    d->taskName = QString(); // Cause name to be re-generated...

    KConfig cfg("taskmanagerrulesrc");
    KConfigGroup grp(&cfg, "Mapping");
    grp.writeEntry(d->task.data()->classClass() + "::" + d->task.data()->className(), url.url());
    cfg.sync();
}

void TaskItem::setLauncherUrl(const AbstractGroupableItem *item)
{
    if (!d->launcherUrl.isEmpty() || !item) {
        return;
    }

    d->launcherUrl = item->launcherUrl();

    if (static_cast<GroupManager *>(parent())->alwaysUseLauncherIcons()) {
        d->launcherIcon = launcherIconFromUrl(d->launcherUrl);
    }

    d->taskName = QString(); // Cause name to be re-generated...
}

static KService::List getServicesViaPid(int pid)
{
    // Attempt to find using commandline...
    KService::List services;

    if (pid == 0) {
        return services;
    }

    KSysGuard::Processes procs;
    procs.updateOrAddProcess(pid);

    KSysGuard::Process *proc = procs.getProcess(pid);
    QString cmdline = proc ? proc->command().simplified() : QString(); // proc->command has a trailing space???

    if (cmdline.isEmpty()) {
        return services;
    }

    const int firstSpace = cmdline.indexOf(' ');

    services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ Exec)").arg(cmdline));
    if (services.empty()) {
        // Could not find with complete commandline, so strip out path part...
        int slash = cmdline.lastIndexOf('/', firstSpace);
        if (slash > 0) {
            services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ Exec)").arg(cmdline.mid(slash + 1)));
        }
    }

    if (services.empty() && firstSpace > 0) {
        // Could not find with arguments, so try without...
        cmdline = cmdline.left(firstSpace);
        services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ Exec)").arg(cmdline));

        int slash = cmdline.lastIndexOf('/');
        if (slash > 0) {
            services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ Exec)").arg(cmdline.mid(slash + 1)));
        }
    }

    if (services.empty() && !QStandardPaths::findExecutable(cmdline).isEmpty()) {
        // cmdline now exists without arguments if there were any
        services << QExplicitlySharedDataPointer<KService>(new KService(proc->name(), cmdline, QString()));
    }
    return services;
}

static QUrl getServiceLauncherUrl(int pid, const QString &type, const QStringList &cmdRemovals = QStringList())
{
    if (pid == 0) {
        return QUrl();
    }

    KSysGuard::Processes procs;
    procs.updateOrAddProcess(pid);

    KSysGuard::Process *proc = procs.getProcess(pid);
    QString cmdline = proc ? proc->command().simplified() : QString(); // proc->command has a trailing space???

    if (cmdline.isEmpty()) {
        return QUrl();
    }

    foreach (const QString & r, cmdRemovals) {
        cmdline.replace(r, "");
    }

    KService::List services = KServiceTypeTrader::self()->query(type, QString("exist Exec and ('%1' =~ Exec)").arg(cmdline));

    if (services.empty()) {
        // Could not find with complete commandline, so strip out path part...
        int slash = cmdline.lastIndexOf('/', cmdline.indexOf(' '));
        if (slash > 0) {
            services = KServiceTypeTrader::self()->query(type, QString("exist Exec and ('%1' =~ Exec)").arg(cmdline.mid(slash + 1)));
        }

        if (services.empty()) {
            return QUrl();
        }
    }

    QString path = services[0]->entryPath();
    if (!QDir::isAbsolutePath(path)) {
        QString absolutePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kservices5/"+path);
        if (!absolutePath.isEmpty())
            path = absolutePath;
    }

    if (QFile::exists(path)) {
        return QUrl::fromLocalFile(path);
    }

    return QUrl();
}

QUrl TaskItem::launcherUrl() const
{
    if (!d->task && !isStartupItem()) {
        return QUrl();
    }

    if (!d->launcherUrl.isEmpty() || d->checkedForLauncher) {
        return d->launcherUrl;
    }

    // Set a flag so that we remeber that we have already checked for a launcher. This is becasue if we fail, then
    // we will keep on failing so the isEmpty()  check above is not enough.
    d->checkedForLauncher = true;

    if (d->task || d->startupTask) {
        d->launcherUrl = launcherUrlFromTask(static_cast<GroupManager *>(parent()), d->task.data(), d->startupTask.data());
    }

    return d->launcherUrl;
}

QUrl TaskItem::launcherUrlFromTask(GroupManager *groupManager, Task *task, Startup *startup)
{
    QUrl launcherUrl;

    // Search for applications which are executable and case-insensitively match the windowclass of the task and
    // See http://techbase.kde.org/Development/Tutorials/Services/Traders#The_KTrader_Query_Language
    KService::List services;
    bool triedPid = false;

    if (task && !(task->classClass().isEmpty() && task->className().isEmpty())) {

        // For KCModules, if we matched on window class, etc, we would end up matching to kcmshell4 - but we are more than likely
        // interested in the actual control module. Therefore we obtain this via the commandline. This commandline may contain
        // "kdeinit4:" or "[kdeinit]", so we remove these first.
        if ("Kcmshell4" == task->classClass()) {
            launcherUrl = getServiceLauncherUrl(task->pid(), "KCModule", QStringList() << "kdeinit4:" << "[kdeinit]");
            if (!launcherUrl.isEmpty()) {
                return launcherUrl;
            }
        }

        // Check to see if this wmClass matched a saved one...
        KConfig cfg("taskmanagerrulesrc");
        KConfigGroup grp(&cfg, "Mapping");
        KConfigGroup set(&cfg, "Settings");

        // Some apps have different launchers depending upon commandline...
        QStringList matchCommandLineFirst = set.readEntry("MatchCommandLineFirst", QStringList());
        if (!task->classClass().isEmpty() && matchCommandLineFirst.contains(task->classClass())) {
            triedPid = true;
            services = getServicesViaPid(task->pid());
        }
        // Try to match using className also
        if (!task->className().isEmpty() && matchCommandLineFirst.contains("::"+task->className())) {
            triedPid = true;
            services = getServicesViaPid(task->pid());
        }

        // If the user has manualy set a mapping, respect this first...
        QString mapped(grp.readEntry(task->classClass() + "::" + task->className(), QString()));

        if (mapped.endsWith(".desktop")) {
            launcherUrl = mapped;
            return launcherUrl;
        }

        if (!task->classClass().isEmpty()) {
            if (mapped.isEmpty()) {
                mapped = grp.readEntry(task->classClass(), QString());

                if (mapped.endsWith(".desktop")) {
                    launcherUrl = mapped;
                    return launcherUrl;
                }
            }

            // Some apps, such as Wine, cannot use className to map to launcher name - as Wine itself is not a GUI app
            // So, Settings/ManualOnly lists window classes where the user will always have to manualy set the launcher...
            QStringList manualOnly = set.readEntry("ManualOnly", QStringList());

            if (!task->classClass().isEmpty() && manualOnly.contains(task->classClass())) {
                return launcherUrl;
            }

            if (!mapped.isEmpty()) {
                services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ DesktopEntryName)").arg(mapped));
            }

            if (!mapped.isEmpty() && services.empty()) {
                services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ Name)").arg(mapped));
            }

            if (services.empty() && groupManager) {
                QUrl savedUrl = groupManager->launcherForWmClass(task->classClass());
                if (savedUrl.isValid()) {
                    launcherUrl = savedUrl;
                    return launcherUrl;
                }
            }

            // To match other docks (docky, unity, etc.) attempt to match on DesktopEntryName first...
            if (services.empty()) {
                services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ DesktopEntryName)").arg(task->classClass()));
            }

            // Try StartupWMClass
            if (services.empty()) {
                services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ StartupWMClass)").arg(task->classClass()));
            }

            // Try 'Name' - unfortunately this can be translated, so has a good chance of failing! (As it does for KDE's own "System Settings" (even in English!!))
            if (services.empty()) {
                services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ Name)").arg(task->classClass()));
            }
        }

        // Ok, absolute *last* chance, try matching via pid (but only if we have not already tried this!)...
        if (services.empty() && !triedPid) {
            services = getServicesViaPid(task->pid());
        }
    }

    if (services.empty() && startup) {
        // Try to match via desktop filename...
        if (!startup->desktopId().isNull() && startup->desktopId().endsWith(".desktop")) {
            if (startup->desktopId().startsWith("/")) {
                launcherUrl = QUrl::fromLocalFile(startup->desktopId());
                return launcherUrl;
            } else {
                QString desktopName = startup->desktopId();

                if (desktopName.endsWith(".desktop")) {
                    desktopName = desktopName.mid(desktopName.length() - 8);
                }

                services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ DesktopEntryName)").arg(desktopName));
            }
        }

        // Try StartupWMClass
        if (services.empty() && !startup->wmClass().isNull()) {
            services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ StartupWMClass)").arg(startup->wmClass()));
        }

        // Try via name...
        if (services.empty() && !startup->text().isNull()) {
            services = KServiceTypeTrader::self()->query("Application", QString("exist Exec and ('%1' =~ Name)").arg(startup->text()));
        }
    }

    if (!services.empty()) {
        QString path = services[0]->entryPath();
        if (path.isEmpty()) {
            path = services[0]->exec();
        }

        if (!path.isEmpty()) {
            launcherUrl = QUrl::fromLocalFile(path);
        }
    }

    return launcherUrl;
}

QIcon TaskItem::launcherIconFromUrl(const QUrl &url)
{
    if (url.isLocalFile() && KDesktopFile::isDesktopFile(url.toLocalFile())) {
        KDesktopFile f(url.toLocalFile());

        if (f.tryExec()) {
            return QIcon::fromTheme(f.readIcon());
        }
    } else if (url.scheme() == "preferred") {
        //NOTE: preferred is NOT a protocol, it's just a magic string
        const KService::Ptr service = KService::serviceByStorageId(LauncherItem::defaultApplication(url));

        if (service) {
            QString desktopFile = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, service->entryPath());
            KDesktopFile f(desktopFile);

            return QIcon::fromTheme(f.readIcon());
        }
    } else {
        const KFileItem fileItem(url);
        return QIcon::fromTheme(fileItem.iconName());
    }

    return QIcon();
}

void TaskItem::resetLauncherCheck()
{
    if (d->launcherUrl.isEmpty()) {
        d->checkedForLauncher = false;
    }
}

void TaskItem::close()
{
    if (!d->task) {
        return;
    }
    d->task.data()->close();
}

bool TaskItem::isActive() const
{
    if (!d->task) {
        return false;
    }
    return d->task.data()->isActive();
}

bool TaskItem::demandsAttention() const
{
    if (!d->task) {
        return false;
    }
    return d->task.data()->demandsAttention();
}

} // TaskManager namespace

#include "moc_taskitem.cpp"
