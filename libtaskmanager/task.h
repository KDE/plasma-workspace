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

#ifndef TASK_H
#define TASK_H

// Qt
#include <QtGui/QDrag>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>
#include <QtGui/QIcon>

// KDE
#include <KWindowSystem>

#include <taskmanager.h>
#include <taskmanager_export.h>
#include <config-X11.h>

class NETWinInfo;

namespace TaskManager
{

/**
 * A dynamic interface to a task (main window).
 *
 * @see TaskManager
 */
class TASKMANAGER_EXPORT Task : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString visibleName READ visibleName)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString className READ className)
    Q_PROPERTY(QString visibleNameWithState READ visibleNameWithState)
    Q_PROPERTY(QPixmap pixmap READ pixmap)
    Q_PROPERTY(bool maximized READ isMaximized)
    Q_PROPERTY(bool minimized READ isMinimized)
    // KDE4 deprecated
    Q_PROPERTY(bool iconified READ isIconified)
    Q_PROPERTY(bool shaded READ isShaded WRITE setShaded)
    Q_PROPERTY(bool active READ isActive)
    Q_PROPERTY(bool onCurrentDesktop READ isOnCurrentDesktop)
    Q_PROPERTY(bool onAllDesktops READ isOnAllDesktops)
    Q_PROPERTY(bool alwaysOnTop READ isAlwaysOnTop WRITE setAlwaysOnTop)
    Q_PROPERTY(bool modified READ isModified)
    Q_PROPERTY(bool demandsAttention READ demandsAttention)
    Q_PROPERTY(int desktop READ desktop)
    Q_PROPERTY(bool onCurrentActivity READ isOnCurrentActivity)
    Q_PROPERTY(bool onAllActivities READ isOnAllActivities)
    Q_PROPERTY(QStringList activities READ activities)

public:

    /**
     * Represents all the informations reported by KWindowSystem about the NET properties of the task
     */
    struct WindowProperties {
        /**
         * This is corresponding to NET::Property enum reported properties.
         */
        unsigned long netWindowInfoProperties;
        /**
         * This is corresponding to NET::Property2 enum reported properties.
         */
        unsigned long netWindowInfoProperties2;

        WindowProperties(unsigned int netWinInfoProperties, unsigned int netWinInfoProperties2);
    };

    Task(WId win, QObject *parent, const char *name = 0);
    virtual ~Task();

    WId window() const;
    KWindowInfo info() const;

    QString visibleName() const;
    QString visibleNameWithState() const;
    QString name() const;
    QString className() const;
    QString classClass() const;
    int pid() const;

    /**
     * A list of the window ids of all transient windows (dialogs) associated
     * with this task.
     */
    WindowList transients() const;

    /**
     * Returns a 16x16 (KIconLoader::Small) icon for the task. This method will
     * only fall back to a static icon if there is no icon of any size in
     * the WM hints.
     */
    QPixmap pixmap() const;

    /**
     * Returns the best icon for any of the KIconLoader::StdSizes. If there is no
     * icon of the specified size specified in the WM hints, it will try to
     * get one using KIconLoader.
     *
     * <pre>
     *   bool gotStaticIcon;
     *   QPixmap icon = myTask->icon( KIconLoader::SizeMedium, gotStaticIcon );
     * </pre>
     *
     * @param size Any of the constants in KIconLoader::StdSizes.
     * @param isStaticIcon Set to true if KIconLoader was used, false otherwise.
     */
    QPixmap bestIcon(int size, bool &isStaticIcon);

    /**
     * Tries to find an icon for the task with the specified size. If there
     * is no icon that matches then it will either resize the closest available
     * icon or return a null pixmap depending on the value of allowResize.
     *
     * Note that the last icon is cached, so a sequence of calls with the same
     * parameters will only query the NET properties if the icon has changed or
     * none was found.
     */
    QPixmap icon(int width, int height, bool allowResize = false);

    /**
     * \return a QIcon for the task
     */
    QIcon icon();

    /**
     * Returns true iff the windows with the specified ids should be grouped
     * together in the task list.
     */
    static bool idMatch(const QString &, const QString &);

    // state

    /**
     * Returns true if the task's window is maximized.
     */
    bool isMaximized() const;

    /**
     * Returns true if the task's window is minimized.
     */
    bool isMinimized() const;

    /**
     * @deprecated
     * Returns true if the task's window is minimized(iconified).
     */
    bool isIconified() const;

    /**
     * Returns true if the task's window is shaded.
     */
    bool isShaded() const;

    /**
     * Returns true if the task's window is the active window.
     */
    bool isActive() const;

    /**
     * Returns true if the task's window is the topmost non-iconified,
     * non-always-on-top window.
     */
    bool isOnTop() const;

    /**
     * Returns true if the task's window is on the current virtual desktop.
     */
    bool isOnCurrentDesktop() const;

    /**
     * Returns true if the task's window is on all virtual desktops.
     */
    bool isOnAllDesktops() const;

    /**
     * Returns true if the task's window will remain at the top of the
     * stacking order.
     */
    bool isAlwaysOnTop() const;

    /**
     * Returns true if the task's window will remain at the bottom of the
     * stacking order.
     */
    bool isKeptBelowOthers() const;

    /**
     * Returns true if the task's window is in full screen mode
     */
    bool isFullScreen() const;

    /**
     * Returns true if the document the task is editing has been modified.
     * This is currently handled heuristically by looking for the string
     * '[i18n_modified]' in the window title where i18n_modified is the
     * word 'modified' in the current language.
     */
    bool isModified() const ;

    /**
     * Returns the desktop on which this task's window resides.
     */
    int desktop() const;

    /**
     * Returns true if the task is not active but demands user's attention.
     */
    bool demandsAttention() const;


    /**
    * Returns true if the window is on the specified screen of a multihead configuration
    */
    bool isOnScreen(int screen) const;

    /**
     * Returns the screen the largest part of this window is on (or -1 if not on any)
     */
    int screen() const;

    /**
     * Returns true if the task should be shown in taskbar-like apps
     */
    bool showInTaskbar() const;

    /**
     * Returns true if the task should be shown in pager-like apps
     */
    bool showInPager() const;

    /**
     * Returns the geometry for this window
     */
    QRect geometry() const;

    /**
     * Returns true if the task's window is on the current activity.
     */
    bool isOnCurrentActivity() const;

    /**
     * Returns true if the task's window is on all activities
     */
    bool isOnAllActivities() const;

    /**
     * Returns the activities on which this task's window resides.
     */
    QStringList activities() const;

    // internal

    //* @internal
    ::TaskManager::TaskChanges refresh(WindowProperties dirty);
    //* @internal
#if HAVE_X11
    void addTransient(WId w, const KWindowInfo &info);
#endif
    //* @internal
    void removeTransient(WId w);
    //* @internal
    bool hasTransient(WId w) const;
    //* @internal
    bool updateDemandsAttentionState(WId w);
    //* @internal
    void setActive(bool a);

    /**
     * Adds the identifying information for this task to mime data for drags, copies, etc
     */
    void addMimeData(QMimeData *mimeData) const;

    /**
     * Returns the mimetype used for a Task
     */
    static QString mimetype();

    /**
     * Returns the mimetype used for multiple Tasks
     */
    static QString groupMimetype();

    /**
     * Given mime data, will return a WId if it can decode one from the data. Otherwise
     * returns 0.
     */
    static WId idFromMimeData(const QMimeData *mimeData, bool *ok = 0);

    /**
     * Given mime data, will return a QList<WId> if it can decode WIds from the data. Otherwise
     * returns an empty list.
     */
    static QList<WId> idsFromMimeData(const QMimeData *mimeData, bool *ok = 0);

public Q_SLOTS:
    /**
     * Maximise the main window of this task.
     */
    void setMaximized(bool);
    void toggleMaximized();

    /**
     * Restore the main window of the task (if it was iconified).
     */
    void restore();

    /**
     * Move the window of this task.
     */
    void move();

    /**
     * Resize the window of this task.
     */
    void resize();

    /**
     * Iconify the task.
     */
    void setIconified(bool);
    void toggleIconified();

    /**
     * Close the task's window.
     */
    void close();

    /**
     * Raise the task's window.
     */
    void raise();

    /**
     * Lower the task's window.
     */
    void lower();

    /**
      * Activate the task's window.
      */
    void activate();

    /**
     * Perform the action that is most appropriate for this task. If it
     * is not active, activate it. Else if it is not the top window, raise
     * it. Otherwise, iconify it.
     */
    void activateRaiseOrIconify();

    /**
     * If true, the task's window will remain at the top of the stacking order.
     */
    void setAlwaysOnTop(bool);
    void toggleAlwaysOnTop();

    /**
     * If true, the task's window will remain at the bottom of the stacking order.
     */
    void setKeptBelowOthers(bool);
    void toggleKeptBelowOthers();

    /**
     * If true, the task's window will enter full screen mode.
     */
    void setFullScreen(bool);
    void toggleFullScreen();

    /**
     * If true then the task's window will be shaded. Most window managers
     * represent this state by displaying on the window's title bar.
     */
    void setShaded(bool);
    void toggleShaded();

    /**
     * Moves the task's window to the specified virtual desktop.
     */
    void toDesktop(int);

    /**
     * Moves the task's window to the current virtual desktop.
     */
    void toCurrentDesktop();

    /**
     * This method informs the window manager of the location at which this
     * task will be displayed when iconised. It is used, for example by the
     * KWin inconify animation.
     */
    void publishIconGeometry(QRect);

    /**
     * Releases pixmap objects; useful to clear out pixmap usage prior to application stoppage
     */
    void clearPixmapData();

Q_SIGNALS:
    /**
     * Indicates that this task has changed in some way.
     */
    void changed(::TaskManager::TaskChanges change);

    /**
     * Indicates that this task is now the active task.
     */
    void activated();

    /**
     * Indicates that this task is no longer the active task.
     */
    void deactivated();

protected:
    void findWindowFrameId();
    void timerEvent(QTimerEvent *event);
    //* @internal */
    void refreshIcon();
    void refreshActivities();

private:
    class Private;
    Private * const d;
};

} // TaskManager namespace

#endif
