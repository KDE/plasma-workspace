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

#ifndef LEGACYTASKMANAGER_H
#define LEGACYTASKMANAGER_H

#include <QHash>
#include <QSet>
#include <QVector>

#include <KStartupInfo>

class QUuid;

namespace LegacyTaskManager
{
typedef QSet<WId> WindowList;

class Task;

class Startup;

enum TaskChange { TaskUnchanged = 0,
                  NameChanged = 1,
                  StateChanged = 2,
                  DesktopChanged = 32,
                  GeometryChanged = 64,
                  WindowTypeChanged = 128,
                  ActionsChanged = 256,
                  TransientsChanged = 512,
                  IconChanged = 1024,
                  ActivitiesChanged = 4096,
                  AttentionChanged = 8192,
                  ClassChanged = 0x4000,
                  EverythingChanged = 0xffff
                };
Q_DECLARE_FLAGS(TaskChanges, TaskChange)
} // namespace LegacyTaskManager

// Own
#include <startup.h>
#include <task.h>
#include <legacytaskmanager_export.h>

namespace LegacyTaskManager
{

/**
 * A generic API for task managers. This class provides an easy way to
 * build NET compliant task managers. It provides support for startup
 * notification, virtual desktops and the full range of WM properties.
 *
 * @see Task
 * @see Startup
 */
class LEGACYTASKMANAGER_EXPORT LegacyTaskManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentDesktop READ currentDesktop)
    Q_PROPERTY(int numberOfDesktops READ numberOfDesktops)
    Q_PROPERTY(QString currentActivity READ currentActivity NOTIFY activityChanged)

public:
    static LegacyTaskManager* self();

    LegacyTaskManager();
    ~LegacyTaskManager() override;

    /**
     * Returns the task for a given WId, or 0 if there is no such task.
     */
    Task *findTask(WId w);

    /**
     * Returns the task for a given location, or 0 if there is no such task.
     */
    Task *findTask(int desktop, const QPoint& p);

    /**
     * Returns a list of all current tasks.
     */
    QHash<WId, Task *> tasks() const;

    /**
     * Returns a list of all current startups.
     */
    QList<Startup *> startups() const;

    /**
     * Returns the name of the nth desktop.
     */
    QString desktopName(int n) const;

    /**
     * Returns the number of virtual desktops.
     */
    int numberOfDesktops() const;

    /**
     * Returns the number of the current desktop.
     */
    int currentDesktop() const;

    /**
     * Returns the number of the current desktop.
     */
    QString currentActivity() const;

    /**
     * Returns true if the specified task is on top.
     */
    bool isOnTop(const Task*) const;

    /**
     * Tells the task manager whether or not we care about geometry
     * updates. This generates a lot of activity so should only be used
     * when necessary.
     */
    void setTrackGeometry(bool track, const QUuid &token);

    /**
     * @return true if geometry tracking is on
     */
    bool trackGeometry() const;

Q_SIGNALS:
    /**
     * Emitted when a new task has started.
     */
    void taskAdded(::LegacyTaskManager::Task *task);

    /**
     * Emitted when a task has terminated.
     */
    void taskRemoved(::LegacyTaskManager::Task *task);

    /**
     * Emitted when a new task is expected.
     */
    void startupAdded(::LegacyTaskManager::Startup *startup);

    /**
     * Emitted when a startup item should be removed. This could be because
     * the task has started, because it is known to have died, or simply
     * as a result of a timeout.
     */
    void startupRemoved(::LegacyTaskManager::Startup *startup);

    /**
     * Emitted when the current desktop changes.
     */
    void desktopChanged(int desktop);

    /**
     * Emitted when the current activity changes.
     */
    void activityChanged(const QString &activity);

    /**
     * Emitted when a window changes desktop.
     */
    void windowChanged(::LegacyTaskManager::Task *task, ::LegacyTaskManager::TaskChanges change);

protected Q_SLOTS:
    //* @internal
    void windowAdded(WId);
    //* @internal
    void windowRemoved(WId);
    //* @internal
    void windowChanged(WId, const unsigned long*);

    //* @internal
    void activeWindowChanged(WId);
    //* @internal
    void currentDesktopChanged(int);
    //* @internal
    void killStartup(const KStartupInfoId&);

    //* @internal
    void gotNewStartup(const KStartupInfoId&, const KStartupInfoData&);
    //* @internal
    void gotStartupChange(const KStartupInfoId&, const KStartupInfoData&);
    //* @internal
    void startupDestroyed(QObject*);

    //* @internal
    void taskChanged(::LegacyTaskManager::TaskChanges changes);

protected Q_SLOTS:
    void configureStartup();

private:
    class Private;
    Private * const d;

    Q_PRIVATE_SLOT(d, void onAppExitCleanup())
};

} // LegacyTaskManager namespace

Q_DECLARE_OPERATORS_FOR_FLAGS(LegacyTaskManager::TaskChanges)

#endif
