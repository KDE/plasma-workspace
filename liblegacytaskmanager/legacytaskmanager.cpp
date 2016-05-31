/*****************************************************************

Copyright (c) 2000 Matthias Elter <elter@kde.org>

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
#include "legacytaskmanager.h"

// Qt
#include <QApplication>
#include <QTimer>
#include <QUuid>
#include <QScreen>

// KDE
#include <KConfig>
#include <KConfigGroup>
#include <KDirWatch>
#include <netwm.h>

#include <config-workspace.h>

#if HAVE_X11
#include <QX11Info>
#endif

#ifdef Q_WS_WIN
#include <windows.h>
#endif

#include <KActivities/Consumer>

namespace LegacyTaskManager
{

class LegacyTaskManagerSingleton
{
public:
    LegacyTaskManager self;
};

Q_GLOBAL_STATIC(LegacyTaskManagerSingleton, privateTaskManagerSelf)

static const int s_startupDefaultTimeout = 5;

LegacyTaskManager* LegacyTaskManager::self()
{
    return &privateTaskManagerSelf->self;
}

class LegacyTaskManager::Private
{
public:
    Private(LegacyTaskManager *manager)
        : q(manager),
          active(0),
          startupInfo(0),
          watcher(0) {
    }

    void onAppExitCleanup() {
        q->disconnect(KWindowSystem::self(), 0, q, 0);
        delete watcher;
        watcher = 0;
        delete startupInfo;
        startupInfo = 0;

        foreach (Task *task, tasksByWId) {
            task->clearPixmapData();
        }

        foreach (Startup *startup, startups) {
            startup->clearPixmapData();
        }
    }

    LegacyTaskManager *q;
    Task *active;
    KStartupInfo* startupInfo;
    KDirWatch *watcher;
    QHash<WId, Task *> tasksByWId;
    QList<Startup *> startups;
    WindowList skiptaskbarWindows;
    QSet<QUuid> trackGeometryTokens;
    KActivities::Consumer activityConsumer;
};

LegacyTaskManager::LegacyTaskManager()
    : QObject(),
      d(new Private(this))
{
    connect(KWindowSystem::self(), &KWindowSystem::windowAdded,
            this,       &LegacyTaskManager::windowAdded);
    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved,
            this,       &LegacyTaskManager::windowRemoved);
    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged,
            this,       &LegacyTaskManager::activeWindowChanged);
    connect(KWindowSystem::self(), &KWindowSystem::currentDesktopChanged,
            this,       &LegacyTaskManager::currentDesktopChanged);
    connect(KWindowSystem::self(), SIGNAL(windowChanged(WId, const ulong*)),
            this,       SLOT(windowChanged(WId, const ulong*)));
    connect(&d->activityConsumer, &KActivities::Consumer::currentActivityChanged,
            this,       &LegacyTaskManager::activityChanged);
    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(onAppExitCleanup()));
    }

    emit activityChanged(d->activityConsumer.currentActivity());

    // register existing windows
    const QList<WId> windows = KWindowSystem::windows();
    QList<WId>::ConstIterator end(windows.end());
    for (QList<WId>::ConstIterator it = windows.begin(); it != end; ++it) {
        windowAdded(*it);
    }

    // set active window
    WId win = KWindowSystem::activeWindow();
    activeWindowChanged(win);

    d->watcher = new KDirWatch(this);
    d->watcher->addFile(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/klaunchrc");
    connect(d->watcher, &KDirWatch::dirty, this, &LegacyTaskManager::configureStartup);
    connect(d->watcher, &KDirWatch::created, this, &LegacyTaskManager::configureStartup);
    connect(d->watcher, &KDirWatch::deleted, this, &LegacyTaskManager::configureStartup);

    configureStartup();
}

LegacyTaskManager::~LegacyTaskManager()
{
    QHash<WId, Task *> tasksByWId = d->tasksByWId;
    d->tasksByWId.clear();
    qDeleteAll(tasksByWId);

    QList<Startup *> startups = d->startups;
    d->startups.clear();
    qDeleteAll(startups);

    delete d;
}

void LegacyTaskManager::configureStartup()
{
    KConfig _c(QStringLiteral("klaunchrc"));
    KConfigGroup c(&_c, "FeedbackStyle");
    if (!c.readEntry("TaskbarButton", true)) {
        delete d->startupInfo;
        d->startupInfo = 0;
        return;
    }

    if (!d->startupInfo) {
        d->startupInfo = new KStartupInfo(KStartupInfo::CleanOnCantDetect, this);
        connect(d->startupInfo,
                &KStartupInfo::gotNewStartup,
                this, &LegacyTaskManager::gotNewStartup);
        connect(d->startupInfo,
                &KStartupInfo::gotStartupChange,
                this, &LegacyTaskManager::gotStartupChange);
        connect(d->startupInfo,
                &KStartupInfo::gotRemoveStartup,
                this, &LegacyTaskManager::killStartup);
    }

    c = KConfigGroup(&_c, "TaskbarButtonSettings");
    d->startupInfo->setTimeout(c.readEntry("Timeout", s_startupDefaultTimeout));
}

Task *LegacyTaskManager::findTask(WId w)
{
    QHashIterator<WId, Task *> it (d->tasksByWId);

    while (it.hasNext()) {
        it.next();
        if (it.key() == w || it.value()->hasTransient(w)) {
            return it.value();
        }
    }

    return 0;
}

Task *LegacyTaskManager::findTask(int desktop, const QPoint& p)
{
    QList<WId> list = KWindowSystem::stackingOrder();

    Task *task = 0;
    int currentIndex = -1;
    foreach (Task *t, d->tasksByWId) {
        if (!t->isOnAllDesktops() && t->desktop() != desktop) {
            continue;
        }
        //FIXME activities?

        if (t->isIconified() || t->isShaded()) {
            continue;
        }

        if (t->geometry().contains(p)) {
            int index = list.indexOf(t->window());
            if (index > currentIndex) {
                currentIndex = index;
                task = t;
            }
        }
    }

    return task;
}

void LegacyTaskManager::windowAdded(WId w)
{
#if HAVE_X11
    KWindowInfo info(w,
                     NET::WMWindowType | NET::WMPid | NET::WMState | NET::WMName,
                     NET::WM2TransientFor);

    // ignore NET::Tool and other special window types
    NET::WindowType wType = info.windowType(NET::NormalMask | NET::DesktopMask | NET::DockMask |
                                            NET::ToolbarMask | NET::MenuMask | NET::DialogMask |
                                            NET::OverrideMask | NET::TopMenuMask |
                                            NET::UtilityMask | NET::SplashMask);

    if (info.transientFor() > 0) {
        const WId transientFor = info.transientFor();

        // check if it's transient for a skiptaskbar window
        if (d->skiptaskbarWindows.contains(transientFor)) {
            return;
        }

        // lets see if this is a transient for an existing task
        if (transientFor != QX11Info::appRootWindow()) {
            Task *t = findTask(transientFor);
            if (t) {
                if (t->window() != w) {
                    t->addTransient(w, info);
                    // qDebug() << "TM: Transient " << w << " added for Task: " << t->window();
                }
                return;
            }
        }
    }

    if (wType != NET::Normal && wType != NET::Override && wType != NET::Unknown &&
        wType != NET::Dialog && wType != NET::Utility) {
        return;
    }

    // ignore windows that want to be ignored by the taskbar
    if ((info.state() & NET::SkipTaskbar) != 0) {
        d->skiptaskbarWindows.insert(w); // remember them though
        return;
    }

#endif

    Task *t = new Task(w, 0);
    d->tasksByWId.insert(w, t);

    connect(t, SIGNAL(changed(::LegacyTaskManager::TaskChanges)),
            this, SLOT(taskChanged(::LegacyTaskManager::TaskChanges)));

    if (d->startupInfo) {
        KStartupInfoId startupInfoId;
        // checkStartup modifies startupInfoId
        d->startupInfo->checkStartup(w, startupInfoId);
        foreach (Startup *startup, d->startups) {
            if (startup->id() == startupInfoId) {
                startup->addWindowMatch(w);
            }
        }
    }

    // qDebug() << "TM: Task added for WId: " << w;
    emit taskAdded(t);
}

void LegacyTaskManager::windowRemoved(WId w)
{
    d->skiptaskbarWindows.remove(w);

    // find task
    Task *t = findTask(w);
    if (!t) {
        return;
    }

    if (t->window() == w) {
        d->tasksByWId.remove(w);
        emit taskRemoved(t);

        if (t == d->active) {
            d->active = 0;
        }

        //qDebug() << "TM: Task for WId " << w << " removed.";
        // FIXME: due to a bug in Qt 4.x, the event loop reference count is incorrect
        // when going through x11EventFilter .. :/ so we have to singleShot the deleteLater
        QTimer::singleShot(0, t, &QObject::deleteLater);
    } else {
        t->removeTransient(w);
        //qDebug() << "TM: Transient " << w << " for Task " << t->window() << " removed.";
    }
}

void LegacyTaskManager::windowChanged(WId w, const unsigned long *dirty)
{
#if HAVE_X11
    if (dirty[NETWinInfo::PROTOCOLS] & NET::WMState) {
        NETWinInfo info(QX11Info::connection(), w, QX11Info::appRootWindow(),
                        NET::WMState | NET::XAWMState);

        if (info.state() & NET::SkipTaskbar) {
            windowRemoved(w);
            d->skiptaskbarWindows.insert(w);
            return;
        } else {
            d->skiptaskbarWindows.remove(w);
            if (info.mappingState() != NET::Withdrawn && !findTask(w)) {
                // skipTaskBar state was removed and the window is still
                // mapped, so add this window
                windowAdded(w);
            }
        }
    }

    // check if any state we are interested in is marked dirty
    if (!(dirty[NETWinInfo::PROTOCOLS] & (NET::WMVisibleName | NET::WMName |
                                          NET::WMState | NET::WMIcon |
                                          NET::XAWMState | NET::WMDesktop) ||
            (trackGeometry() && dirty[NETWinInfo::PROTOCOLS] & NET::WMGeometry) ||
            (dirty[NETWinInfo::PROTOCOLS2] & NET::WM2Activities))) {
        return;
    }

    // find task
    Task *t = findTask(w);
    if (!t) {
        return;
    }

    //qDebug() << "LegacyTaskManager::windowChanged " << w << " " << dirty[NETWinInfo::PROTOCOLS] << dirty[NETWinInfo::PROTOCOLS2];

    unsigned long propagatedChanges = 0;
    if ((dirty[NETWinInfo::PROTOCOLS] & NET::WMState) && t->updateDemandsAttentionState(w)) {
        propagatedChanges = NET::WMState;
    }

    //qDebug() << "got changes, but will we refresh?" << dirty[NETWinInfo::PROTOCOLS] << dirty[NETWinInfo::PROTOCOLS2];
    if (dirty[NETWinInfo::PROTOCOLS] || dirty[NETWinInfo::PROTOCOLS2]) {
        // only refresh this stuff if we have other changes besides icons
        t->refresh(Task::WindowProperties(dirty[NETWinInfo::PROTOCOLS] | propagatedChanges, dirty[NETWinInfo::PROTOCOLS2]));
    }
#endif
}

void LegacyTaskManager::taskChanged(::LegacyTaskManager::TaskChanges changes)
{
    Task *t = qobject_cast<Task*>(sender());

    if (!t || changes == TaskUnchanged || !d->tasksByWId.contains(t->info().win())) {
        return;
    }

    emit windowChanged(d->tasksByWId[t->info().win()], changes);
}

void LegacyTaskManager::activeWindowChanged(WId w)
{
    //qDebug() << "LegacyTaskManager::activeWindowChanged" << w;
    Task *t = findTask(w);
    if (!t) {
        if (d->active) {
            d->active->setActive(false);
            d->active = 0;
        }
        //qDebug() << "no active window";
    } else {
        if (t->info().windowType(NET::UtilityMask) == NET::Utility && !t->demandsAttention()) {
            // we don't want to mark utility windows as active since task managers
            // actually care about the main window and skip utility windows; utility
            // windows are hidden when their associated window loses focus anyways
            // see http://bugs.kde.org/show_bug.cgi?id=178509
            return;
        }

        if (d->active) {
            d->active->setActive(false);
        }

        d->active = t;
        d->active->setActive(true);
        //qDebug() << "active window is" << t->name();
    }
}

void LegacyTaskManager::currentDesktopChanged(int desktop)
{
    emit desktopChanged(desktop);
}

void LegacyTaskManager::gotNewStartup(const KStartupInfoId& id, const KStartupInfoData& data)
{
    Startup *s = new Startup(id, data, 0);
    connect(s, &QObject::destroyed, this, &LegacyTaskManager::startupDestroyed);
    d->startups.append(s);
    emit startupAdded(s);
}

void LegacyTaskManager::gotStartupChange(const KStartupInfoId& id, const KStartupInfoData& data)
{
    foreach (Startup *startup, d->startups) {
        if (startup->id() == id) {
            startup->update(data);
            return;
        }
    }
}

void LegacyTaskManager::killStartup(const KStartupInfoId& id)
{
    foreach (Startup *startup, d->startups) {
        if (startup->id() == id) {
            d->startups.removeAll(startup);
            emit startupRemoved(startup);
        }
    }
}

void LegacyTaskManager::startupDestroyed(QObject* obj)
{
    d->startups.removeAll(static_cast<Startup *>(obj));
}

QString LegacyTaskManager::desktopName(int desk) const
{
    return KWindowSystem::desktopName(desk);
}

QHash<WId, Task *> LegacyTaskManager::tasks() const
{
    return d->tasksByWId;
}

QList<Startup *> LegacyTaskManager::startups() const
{
    return d->startups;
}

int LegacyTaskManager::numberOfDesktops() const
{
    return KWindowSystem::numberOfDesktops();
}

bool LegacyTaskManager::isOnTop(const Task *task) const
{
    if (!task) {
        return false;
    }

    QList<WId> list = KWindowSystem::stackingOrder();
    QListIterator<WId> it(list);
    it.toBack();

    const bool multiscreen = QGuiApplication::screens().count() > 1;
    // we only use taskScreen when there are multiple screens, so we
    // only fetch the value in that case; still, do it outside the loop
    const QRect taskScreen = multiscreen ? task->screen() : QRect();

    while (it.hasPrevious()) {
        const WId top = it.previous();
        Task *t = d->tasksByWId.value(top);

        if (!t) {
            foreach (const WId transient, task->transients()) {
                if (transient == top) {
                    return true;
                }
            }

            continue;
        }

        if (t == task) {
            return true;
        }

        foreach (const WId transient, task->transients()) {
            if (transient == top) {
                return true;
            }
        }

        if (t->isFullScreen() && t->screen() != taskScreen) {
            // it seems window managers always claim that fullscreen
            // windows are stacked above everything else .. even when
            // a window on a different physical screen has input focus
            // so we work around this decision here by only paying attention
            // to fullscreen windows that are on the same screen as us
            continue;
        }

#ifndef Q_WS_WIN
        if (!t->isIconified() && (t->isAlwaysOnTop() == task->isAlwaysOnTop())) {
            return false;
        }
#endif
    }

    return false;
}

void LegacyTaskManager::setTrackGeometry(bool track, const QUuid &token)
{
    if (track) {
        if (!d->trackGeometryTokens.contains(token)) {
            d->trackGeometryTokens.insert(token);
        }
    } else {
        d->trackGeometryTokens.remove(token);
    }
}

bool LegacyTaskManager::trackGeometry() const
{
    return !d->trackGeometryTokens.isEmpty();
}

int LegacyTaskManager::currentDesktop() const
{
    return KWindowSystem::currentDesktop();
}

QString LegacyTaskManager::currentActivity() const
{
    return d->activityConsumer.currentActivity();
}

} // LegacyTaskManager namespace


#include "moc_legacytaskmanager.cpp"
