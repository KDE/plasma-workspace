/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
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

#ifndef SHELLCORONA_H
#define SHELLCORONA_H

#include "plasma/corona.h"

namespace Plasma
{
    class Applet;
} // namespace Plasma

class Activity;
class PanelView;
namespace WorkspaceScripting {
    class DesktopScriptEngine;
}

namespace KActivities {
    class Controller;
}

class ShellCorona : public Plasma::Corona
{
    Q_OBJECT
    Q_PROPERTY(QString shell READ shell WRITE setShell)

public:
    explicit ShellCorona(QObject * parent = 0);
    ~ShellCorona();

    /**
     * Where to save global configuration that doesn't have anything to do with the scene (e.g. views)
     */
    KSharedConfig::Ptr applicationConfig();

    WorkspaceScripting::DesktopScriptEngine * scriptEngine() const;
    /**
     * Ensures we have the necessary containments for every screen
     */
    void checkScreens(bool signalWhenExists = false);

    /**
     * Ensures we have the necessary containments for the given screen
     */
    void checkScreen(int screen, bool signalWhenExists = false);

    void checkDesktop(Activity *activity, bool signalWhenExists, int screen);

    int numScreens() const;
    QRect screenGeometry(int id) const;
    QRegion availableScreenRegion(int id) const;
    QRect availableScreenRect(int id) const;

    PanelView *panelView(Plasma::Containment *containment) const;

public Q_SLOTS:
    /**
     * Request saving applicationConfig on disk, it's event compressed, not immediate
     */
    void requestApplicationConfigSync();

    /**
     * Sets the shell that the corona should display
     */
    void setShell(const QString & shell);

    /**
     * Gets the currently shown shell
     */
    QString shell() const;

protected Q_SLOTS:
    void screenCountChanged(int newCount);
    void screenResized(int screen);
    void workAreaResized(int screen);

    void checkViews();
    void updateScreenOwner(int wasScreen, int isScreen, Plasma::Containment *containment);

    void printScriptError(const QString &error);
    void printScriptMessage(const QString &message);

    /**
     * Loads the layout and performs the needed checks
     */
    void load();

    /**
     * Unloads everything
     */
    void unload();

    /**
     * Loads the default (system wide) layout for this user
     **/
    void loadDefaultLayout();

    /**
     * Execute any update script
     */
    void processUpdateScripts();

    Activity* activity(const QString &id);

    KActivities::Controller *activityController();

private Q_SLOTS:
    void checkLoadingDesktopsComplete();
    void handleContainmentAdded(Plasma::Containment *c);
    void showWidgetExplorer();
    void syncAppConfig();
    void setDashboardShown(bool show);
    void checkActivities();
    void currentActivityChanged(const QString &newActivity);
    void activityAdded(const QString &id);
    void activityRemoved(const QString &id);

private:
    class Private;
    const QScopedPointer<Private> d;
};

#endif // SHELLCORONA_H


