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

#ifndef SHELLCORONA_H
#define SHELLCORONA_H

#include "plasma/corona.h"

#include <Plasma/Package>

namespace KScreen {
class Output;
class Config;
}

namespace Plasma
{
    class Applet;
} // namespace Plasma

class Activity;
class DesktopView;
class PanelView;
class QScreen;

namespace KActivities {
    class Controller;
}

class ShellCorona : public Plasma::Corona
{
    Q_OBJECT
    Q_PROPERTY(QString shell READ shell WRITE setShell)
    Q_PROPERTY(int numScreens READ numScreens)
    Q_CLASSINFO("D-Bus Interface", "org.kde.PlasmaShell")

public:
    explicit ShellCorona(QObject *parent = 0);
    ~ShellCorona();

    /**
     * Where to save global configuration that doesn't have anything to do with the scene (e.g. views)
     */
    KSharedConfig::Ptr applicationConfig();

    int numScreens() const;
    Q_INVOKABLE QRect screenGeometry(int id) const;
    Q_INVOKABLE QRegion availableScreenRegion(int id) const;
    Q_INVOKABLE QRect availableScreenRect(int id) const;

    PanelView *panelView(Plasma::Containment *containment) const;

    KActivities::Controller *activityController();

    //Those two are a bit of an hack but are just for desktop scripting
    Activity *activity(const QString &id);
    void insertActivity(const QString &id, Activity *activity);

    Plasma::Containment *setContainmentTypeForScreen(int screen, const QString &plugin);

    KScreen::Config *screensConfiguration() const;

    QScreen *screenForId(int screenId) const;
    void remove(DesktopView *desktopView);

public Q_SLOTS:
    /**
     * Request saving applicationConfig on disk, it's event compressed, not immediate
     */
    void requestApplicationConfigSync();

    /**
     * Sets the shell that the corona should display
     */
    void setShell(const QString &shell);

    /**
     * Gets the currently shown shell
     */
    QString shell() const;

    ///DBUS methods
    void toggleDashboard();
    void setDashboardShown(bool show);
    void showInteractiveConsole();
    void loadScriptInInteractiveConsole(const QString &script);
    void toggleActivityManager();

    Plasma::Containment *addPanel(const QString &plugin);

protected Q_SLOTS:
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

    int screenForContainment(const Plasma::Containment *containment) const;

    void showAlternativesForApplet(Plasma::Applet *applet);

private Q_SLOTS:
    void createWaitingPanels();
    void handleContainmentAdded(Plasma::Containment *c);
    void toggleWidgetExplorer();
    void syncAppConfig();
    void checkActivities();
    void currentActivityChanged(const QString &newActivity);
    void activityAdded(const QString &id);
    void activityRemoved(const QString &id);
    void checkAddPanelAction(const QStringList &sycocaChanges = QStringList());
    void addPanel();
    void addPanel(QAction *action);
    void containmentDeleted(QObject* cont);
    void populateAddPanelsMenu();

    void outputEnabledChanged();
    void addOutput(KScreen::Output *output);
    void primaryOutputChanged();

    void activityOpened();
    void activityClosed();
    void activityRemoved();
    void desktopContainmentDestroyed(QObject*);
    void showOpenGLNotCompatibleWarning();
    void alternativesVisibilityChanged(bool visible);
    void interactiveConsoleVisibilityChanged(bool visible);

private:
    KScreen::Output *findPrimaryOutput() const;
    QScreen *insertScreen(QScreen *screen, int idx);
    void removeView(int idx);
    bool isOutputRedundant(KScreen::Output *output) const;
    void reconsiderOutputs();
    QList<PanelView *> panelsForScreen(QScreen *screen) const;

#ifndef NDEBUG
    void screenInvariants() const;
#endif

    /**
     * @returns a new containment associated with the specified @p activity and @p screen.
     */
    Plasma::Containment *createContainmentForActivity(const QString &activity, int screenNum);
    void insertContainment(const QString &activity, int screenNum, Plasma::Containment *containment);

    class Private;
    Private * d;
};

#endif // SHELLCORONA_H


