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

#include <QSet>
#include <QTimer>

#include <KPackage/Package>

#include <KScreen/Types>

class Activity;
class DesktopView;
class PanelView;
class QMenu;
class QScreen;

namespace KActivities
{
    class Consumer;
    class Controller;
} // namespace KActivities

namespace KDeclarative
{
    class QmlObject;
} // namespace KDeclarative

namespace KScreen {
    class Output;
} // namespace KScreen

namespace Plasma
{
    class Applet;
} // namespace Plasma

namespace KWayland
{
    namespace Client
    {
        class PlasmaShell;
    }
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

    KPackage::Package lookAndFeelPackage();

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

    KScreen::ConfigPtr screensConfiguration() const;

    QScreen *screenForId(int screenId) const;
    void remove(DesktopView *desktopView);

    /**
     * @returns a new containment associated with the specified @p activity and @p screen.
     */
    Plasma::Containment *createContainmentForActivity(const QString &activity, int screenNum);

    KWayland::Client::PlasmaShell *waylandPlasmaShellInterface() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event);

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

    void nextActivity();
    void previousActivity();
    void stopCurrentActivity();

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
    void addOutput(const KScreen::OutputPtr &output);
    void primaryOutputChanged();

    void activityOpened();
    void activityClosed();
    void activityRemoved();
    void desktopContainmentDestroyed(QObject*);
    void showOpenGLNotCompatibleWarning();
    void alternativesVisibilityChanged(bool visible);
    void interactiveConsoleVisibilityChanged(bool visible);
    void screenRemoved(QScreen* screen);

private:
    QScreen *outputToScreen(const KScreen::OutputPtr &output) const;
    KScreen::OutputPtr screenToOutput(QScreen *screen) const;
    QScreen *insertScreen(QScreen *screen, int idx);
    void removeView(int idx);
    bool isOutputRedundant(const KScreen::OutputPtr &output) const;
    void reconsiderOutputs();
    QList<PanelView *> panelsForScreen(QScreen *screen) const;
    DesktopView* desktopForScreen(QScreen *screen) const;
    void setupWaylandIntegration();

#ifndef NDEBUG
    void screenInvariants() const;
#endif

    void insertContainment(const QString &activity, int screenNum, Plasma::Containment *containment);

    QString m_shell;
    QList<DesktopView *> m_views;
    KActivities::Controller *m_activityController;
    KActivities::Consumer *m_activityConsumer;
    QHash<const Plasma::Containment *, PanelView *> m_panelViews;
    KConfigGroup m_desktopDefaultsConfig;
    QList<Plasma::Containment *> m_waitingPanels;
    QHash<QString, Activity *> m_activities;
    QHash<QString, QHash<int, Plasma::Containment *> > m_desktopContainments;
    QAction *m_addPanelAction;
    QMenu *m_addPanelsMenu;
    KPackage::Package m_lookAndFeelPackage;
    QSet<KScreen::OutputPtr> m_redundantOutputs;
    QList<KDeclarative::QmlObject *> m_alternativesObjects;
    KDeclarative::QmlObject *m_interactiveConsole;

    KScreen::ConfigPtr m_screenConfiguration;
    QTimer m_waitingPanelsTimer;
    QTimer m_appConfigSyncTimer;
    QTimer m_reconsiderOutputsTimer;

    KWayland::Client::PlasmaShell *m_waylandPlasmaShell;
};

#endif // SHELLCORONA_H


