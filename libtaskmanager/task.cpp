/*****************************************************************

Copyright (c) 2000-2001 Matthias Elter <elter@kde.org>
Copyright (c) 2001 Richard Moore <rich@kde.org>

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
#include "task.h"
#include "task_p.h"

// Qt
#include <QMimeData>
#include <QTimer>
#include <QApplication>
#include <QDesktopWidget>

// KDE
#include <KIconLoader>
#include <KLocalizedString>

#include "taskmanager.h"

namespace TaskManager
{

Task::Task(WId w, QObject *parent, const char *name)
    : QObject(parent),
      d(new Private(w))
{
    setObjectName(name);

    // try to load icon via net_wm
    refreshIcon();
    refreshActivities();
}

Task::~Task()
{
    delete d;
}

void Task::timerEvent(QTimerEvent *)
{
    if (d->cachedChanges.netWindowInfoProperties || d->cachedChanges.netWindowInfoProperties2) {
        d->lastUpdate = QTime();
        refresh(d->cachedChanges);
        d->cachedChanges.netWindowInfoProperties = 0;
        d->cachedChanges.netWindowInfoProperties2 = 0;
    }

    killTimer(d->cachedChangesTimerId);
    d->cachedChangesTimerId = 0;
}

void Task::refreshIcon()
{
    // try to load icon via net_wm
    d->pixmap = KWindowSystem::icon(d->win, 16, 16, true);

    // try to guess the icon from the classhint
    if (d->pixmap.isNull()) {
        d->pixmap = KIconLoader::global()->loadIcon(className().toLower(),
                    KIconLoader::Small,
                    KIconLoader::Small,
                    KIconLoader::DefaultState,
                    QStringList(), 0, true);

        // load the icon for X applications
        if (d->pixmap.isNull()) {
            d->pixmap = SmallIcon("xorg");
        }
    }

    d->lastIcon = QPixmap();
    d->icon = QIcon();
    emit changed(IconChanged);
}

::TaskManager::TaskChanges Task::refresh(WindowProperties dirty)
{
    if (!d->lastUpdate.isNull() && d->lastUpdate.elapsed() < 200) {
        d->cachedChanges.netWindowInfoProperties |= dirty.netWindowInfoProperties;
        d->cachedChanges.netWindowInfoProperties2 |= dirty.netWindowInfoProperties2;

        if (!d->cachedChangesTimerId) {
            d->cachedChangesTimerId = startTimer(200 - d->lastUpdate.elapsed());
        }

        return TaskUnchanged;
    }

    d->lastUpdate.restart();
    KWindowInfo info(d->win, windowInfoFlags, windowInfoFlags2);
    TaskChanges changes = TaskUnchanged;

    if (d->info.windowClassClass() != info.windowClassClass() ||
        d->info.windowClassName() != info.windowClassName()) {
        changes |= ClassChanged;
    }

    if (d->info.visibleName() != info.visibleName() ||
        d->info.visibleNameWithState() != info.visibleNameWithState() ||
        d->info.name() != info.name()) {
        changes |= NameChanged;
    }

    d->info = info;

    if (dirty.netWindowInfoProperties & NET::WMState || dirty.netWindowInfoProperties & NET::XAWMState) {
        changes |= StateChanged;
        if (demandsAttention() != d->demandedAttention) {
            d->demandedAttention = !d->demandedAttention;
            changes |= AttentionChanged;
        }
    }

    if (dirty.netWindowInfoProperties & NET::WMDesktop) {
        changes |= DesktopChanged;
    }

    if (dirty.netWindowInfoProperties & NET::WMGeometry) {
        changes |= GeometryChanged;
    }

    if (dirty.netWindowInfoProperties & NET::WMWindowType) {
        changes |= WindowTypeChanged;
    }

    if (dirty.netWindowInfoProperties2 & NET::WM2AllowedActions) {
        changes |= ActionsChanged;
    }

    if (dirty.netWindowInfoProperties & NET::WMIcon) {
        refreshIcon();
    }

    if (dirty.netWindowInfoProperties2 & NET::WM2Activities) {
        refreshActivities();
        changes |= ActivitiesChanged;
    }

    if (changes != TaskUnchanged) {
        emit changed(changes);
    }

    return changes;
}

void Task::setActive(bool a)
{
    d->active = a;

    TaskChanges changes = StateChanged;

    if (demandsAttention() != d->demandedAttention) {
        d->demandedAttention = !d->demandedAttention;
        changes |= AttentionChanged;
    }

    emit changed(changes);

    if (a) {
        emit activated();
    } else {
        emit deactivated();
    }
}

bool Task::isMaximized() const
{
    return d->info.valid(true) && (d->info.state() & NET::Max);
}

bool Task::isMinimized() const
{
    return d->info.valid(true) && d->info.isMinimized();
}

bool Task::isIconified() const
{
    return d->info.valid(true) && d->info.isMinimized();
}

bool Task::isAlwaysOnTop() const
{
    return d->info.valid(true) && (d->info.state() & NET::StaysOnTop);
}

bool Task::isKeptBelowOthers() const
{
    return d->info.valid(true) && (d->info.state() & NET::KeepBelow);
}

bool Task::isFullScreen() const
{
    return d->info.valid(true) && (d->info.state() & NET::FullScreen);
}

bool Task::isShaded() const
{
    return d->info.valid(true) && (d->info.state() & NET::Shaded);
}

bool Task::isOnCurrentDesktop() const
{
    return d->info.valid(true) && d->info.isOnCurrentDesktop();
}

bool Task::isOnAllDesktops() const
{
    return d->info.valid(true) && d->info.onAllDesktops();
}

bool Task::isActive() const
{
    if (d->active) {
        return true;
    }

    const WId activeWindow = KWindowSystem::activeWindow();
    foreach (WId window, d->transients) {
        if (activeWindow == window) {
            return true;
        }
    }

    return false;
}

bool Task::isOnTop() const
{
    return TaskManager::self()->isOnTop(this);
}

bool Task::isModified() const
{
    static QString modStr = QString::fromUtf8("[") +
                            i18nc("marks that a task has been modified", "modified") +
                            QString::fromUtf8("]");
    int modStrPos = d->info.visibleName().indexOf(modStr);

    return (modStrPos != -1);
}

int Task::desktop() const
{
    if (KWindowSystem::numberOfDesktops() < 2) {
        return 0;
    }

    return d->info.desktop();
}

bool Task::demandsAttention() const
{
    return (d->info.valid(true) && (d->info.state() & NET::DemandsAttention)) ||
           !d->transientsDemandingAttention.isEmpty();
}

bool Task::isOnScreen(int screen) const
{
    return TaskManager::isOnScreen(screen, d->win);
}

int Task::screen() const
{
    int rv = -1;
    if (!d->info.valid(true)) {
        return rv;
    }

    QDesktopWidget *desktop = qApp->desktop();
    int area = 0;

    for (int i = 0; i < desktop->screenCount(); ++i) {
        const QRect desktopGeometry = desktop->screenGeometry(i);
        const QRect onScreen = desktopGeometry.intersected(d->info.geometry());
        if (onScreen.height() * onScreen.width() > area) {
            area = onScreen.height() * onScreen.width();
            rv = i;
        }
    }

    return rv;
}

bool Task::showInTaskbar() const
{
    return d->info.state() ^ NET::SkipTaskbar;
}

bool Task::showInPager() const
{
    return d->info.state() ^ NET::SkipPager;
}

QRect Task::geometry() const
{
    return d->info.geometry();
}

void Task::removeTransient(WId w)
{
    d->transients.remove(w);
    d->transientsDemandingAttention.remove(w);
    if (demandsAttention() != d->demandedAttention) {
        d->demandedAttention = !d->demandedAttention;
        emit changed(AttentionChanged);
    }
}

bool Task::hasTransient(WId w) const
{
    return d->transients.contains(w);
}

WId Task::window() const
{
    return d->win;
}

KWindowInfo Task::info() const
{
    return d->info;
}

QString Task::visibleName() const
{
    return d->info.visibleName();
}

QString Task::visibleNameWithState() const
{
    return d->info.visibleNameWithState();
}

QString Task::name() const
{
    return d->info.name();
}

QPixmap Task::icon(int width, int height, bool allowResize)
{
    if (width == d->lastWidth &&
            height == d->lastHeight &&
            allowResize == d->lastResize &&
            !d->lastIcon.isNull()) {
        return d->lastIcon;
    }

    QPixmap newIcon = KWindowSystem::icon(d->win, width, height, allowResize);
    if (!newIcon.isNull()) {
        d->lastIcon = newIcon;
        d->lastWidth = width;
        d->lastHeight = height;
        d->lastResize = allowResize;
    }

    return newIcon;
}

QIcon Task::icon()
{
    if (d->icon.isNull()) {
        d->icon.addPixmap(KWindowSystem::icon(d->win, KIconLoader::SizeSmall, KIconLoader::SizeSmall, false));
        d->icon.addPixmap(KWindowSystem::icon(d->win, KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium, false));
        d->icon.addPixmap(KWindowSystem::icon(d->win, KIconLoader::SizeMedium, KIconLoader::SizeMedium, false));
        d->icon.addPixmap(KWindowSystem::icon(d->win, KIconLoader::SizeLarge, KIconLoader::SizeLarge, false));
    }

    return d->icon;
}

WindowList Task::transients() const
{
    return d->transients;
}

QPixmap Task::pixmap() const
{
    return d->pixmap;
}

QPixmap Task::bestIcon(int size, bool &isStaticIcon)
{
    QPixmap pixmap;
    isStaticIcon = false;

    switch (size) {
    case KIconLoader::SizeSmall: {
        pixmap = icon(16, 16, true);

        // Icon of last resort
        if (pixmap.isNull()) {
            pixmap = KIconLoader::global()->loadIcon("xorg",
                     KIconLoader::NoGroup,
                     KIconLoader::SizeSmall);
            isStaticIcon = true;
        }
    }
    break;
    case KIconLoader::SizeMedium: {
        //
        // Try 34x34 first for KDE 2.1 icons with shadows, if we don't
        // get one then try 32x32.
        //
        pixmap = icon(34, 34, false);

        if (((pixmap.width() != 34) || (pixmap.height() != 34)) &&
                ((pixmap.width() != 32) || (pixmap.height() != 32))) {
            pixmap = icon(32, 32, true);
        }

        // Icon of last resort
        if (pixmap.isNull()) {
            pixmap = KIconLoader::global()->loadIcon("xorg",
                     KIconLoader::NoGroup,
                     KIconLoader::SizeMedium);
            isStaticIcon = true;
        }
    }
    break;
    case KIconLoader::SizeLarge: {
        // If there's a 48x48 icon in the hints then use it
        pixmap = icon(size, size, false);

        // If not, try to get one from the classname
        if (pixmap.isNull() || pixmap.width() != size || pixmap.height() != size) {
            pixmap = KIconLoader::global()->loadIcon(className(),
                     KIconLoader::NoGroup,
                     size,
                     KIconLoader::DefaultState,
                     QStringList(), 0L,
                     true);
            isStaticIcon = true;
        }

        // If we still don't have an icon then scale the one in the hints
        if (pixmap.isNull() || (pixmap.width() != size) || (pixmap.height() != size)) {
            pixmap = icon(size, size, true);
            isStaticIcon = false;
        }

        // Icon of last resort
        if (pixmap.isNull()) {
            pixmap = KIconLoader::global()->loadIcon("xorg",
                     KIconLoader::NoGroup,
                     size);
            isStaticIcon = true;
        }
    }
    }

    return pixmap;
}

bool Task::idMatch(const QString& id1, const QString& id2)
{
    if (id1.isEmpty() || id2.isEmpty())
        return false;

    if (id1.contains(id2) > 0)
        return true;

    if (id2.contains(id1) > 0)
        return true;

    return false;
}

void Task::toggleMaximized()
{
    setMaximized(!isMaximized());
}

void Task::setIconified(bool iconify)
{
//     kDebug() <<" going to iconify" << d->win;
    if (iconify) {
        KWindowSystem::minimizeWindow(d->win);
    } else {
        KWindowInfo info(d->win, NET::WMState | NET::XAWMState | NET::WMDesktop);
        bool on_current = info.isOnCurrentDesktop();

        if (!on_current) {
            KWindowSystem::setCurrentDesktop(info.desktop());
        }

        KWindowSystem::unminimizeWindow(d->win);

        if (!on_current) {
            KWindowSystem::forceActiveWindow(d->win);
        }
    }
}

void Task::toggleIconified()
{
    setIconified(!isIconified());
}

void Task::raise()
{
//    kDebug(1210) << "Task::raise(): " << name();
    KWindowSystem::raiseWindow(d->win);
}

void Task::lower()
{
//    kDebug(1210) << "Task::lower(): " << name();
    KWindowSystem::lowerWindow(d->win);
}

void Task::activate()
{
    WId w = d->win;
    if (!d->transientsDemandingAttention.isEmpty()) {
        WindowList::const_iterator it = d->transientsDemandingAttention.end();
        --it;
        w = *it;
    } else if (!d->transients.isEmpty()) {
        WindowList::const_iterator it = d->transients.end();
        --it;
        KWindowInfo info(*it, NET::WMState | NET::XAWMState | NET::WMDesktop);
        //this is a work around for (at least?) kwin where a shaded transient will prevent the main
        //window from being brought forward unless the transient is actually pulled forward, most
        //easily reproduced by opening a modal file open/save dialog on an app then shading the file
        //dialog and trying to bring the window forward by clicking on it in a tasks widget
        //TODO: do we need to check all the transients for shaded?
        if (info.valid(true) && (info.state() & NET::Shaded)) {
            w = *it;
        }
    }


    //kDebug(1210) << "Task::activate():" << name() << d->win << w;
    KWindowSystem::forceActiveWindow(w);
}

void Task::activateRaiseOrIconify()
{
    //kDebug() << isActive() << isIconified() << isOnTop();
    if (!isActive() || isIconified()) {
        activate();
    } else if (!isOnTop()) {
        raise();
    } else {
        setIconified(true);
    }
}

void Task::toCurrentDesktop()
{
    toDesktop(KWindowSystem::currentDesktop());
}

void Task::toggleAlwaysOnTop()
{
    setAlwaysOnTop(!isAlwaysOnTop());
}

void Task::toggleKeptBelowOthers()
{
    setKeptBelowOthers(!isKeptBelowOthers());
}

void Task::toggleFullScreen()
{
    setFullScreen(!isFullScreen());
}

void Task::toggleShaded()
{
    setShaded(!isShaded());
}

void Task::clearPixmapData()
{
    d->lastIcon = QPixmap();
    d->pixmap = QPixmap();
    d->icon = QIcon();
}

void Task::addMimeData(QMimeData *mimeData) const
{
    Q_ASSERT(mimeData);

    QByteArray data;
    data.resize(sizeof(WId));
    memcpy(data.data(), &d->win, sizeof(WId));
    mimeData->setData(mimetype(), data);
}

QString Task::mimetype()
{
    return "windowsystem/winid";
}

QString Task::groupMimetype()
{
    return "windowsystem/multiple-winids";
}

QList<WId> Task::idsFromMimeData(const QMimeData *mimeData, bool *ok)
{
    Q_ASSERT(mimeData);
    QList<WId> ids;

    if (ok) {
        *ok = false;
    }

    if (!mimeData->hasFormat(groupMimetype())) {
        // try to grab a singular id if it exists
        //kDebug() << "not group type";
        bool singularOk;
        WId id = idFromMimeData(mimeData, &singularOk);

        if (ok) {
            *ok = singularOk;
        }

        if (singularOk) {
            //kDebug() << "and singular failed, too";
            ids << id;
        }

        return ids;
    }

    QByteArray data(mimeData->data(groupMimetype()));
    if ((unsigned int)data.size() < sizeof(int) + sizeof(WId)) {
        //kDebug() << "wrong size" << data.size() << sizeof(int) + sizeof(WId);
        return ids;
    }

    int count = 0;
    memcpy(&count, data.data(), sizeof(int));
    if (count < 1 || (unsigned int)data.size() < sizeof(int) + sizeof(WId) * count) {
        //kDebug() << "wrong size, 2" << data.size() << count << sizeof(int) + sizeof(WId) * count;
        return ids;
    }

    WId id;
    for (int i = 0; i < count; ++i) {
        memcpy(&id, data.data() + sizeof(int) + sizeof(WId) * i, sizeof(WId));
        ids << id;
    }

    if (ok) {
        *ok = true;
    }

    return ids;
}

WId Task::idFromMimeData(const QMimeData *mimeData, bool *ok)
{
    Q_ASSERT(mimeData);

    if (ok) {
        *ok = false;
    }

    if (!mimeData->hasFormat(mimetype())) {
        return 0;
    }

    QByteArray data(mimeData->data(mimetype()));
    if (data.size() != sizeof(WId)) {
        return 0;
    }

    WId id;
    memcpy(&id, data.data(), sizeof(WId));

    if (ok) {
        *ok = true;
    }

    return id;
}

bool Task::isOnCurrentActivity() const
{
    return d->activities.isEmpty() || d->activities.contains(TaskManager::self()->currentActivity());
}

bool Task::isOnAllActivities() const
{
    return d->activities.isEmpty();
}

QStringList Task::activities() const
{
    return d->activities;
}

Task::WindowProperties::WindowProperties(unsigned int netWinInfoProperties, unsigned int netWinInfoProperties2)
    : netWindowInfoProperties(netWinInfoProperties), netWindowInfoProperties2(netWinInfoProperties2)
{
}

} // TaskManager namespace

#include "task.moc"

