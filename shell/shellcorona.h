/*
    SPDX-FileCopyrightText: 2008 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2013 Ivan Cukic <ivan.cukic@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "plasma/corona.h"

#include <QDBusContext>
#include <QDBusVariant>
#include <QScopedPointer>
#include <QSet>
#include <QTimer>

#include <KPackage/Package>

class DesktopView;
class PanelView;
class QMenu;
class QScreen;
class ScreenPool;
class StrutManager;

namespace KActivities
{
class Controller;
} // namespace KActivities

namespace KDeclarative
{
class QmlObjectSharedEngine;
} // namespace KDeclarative

namespace KScreen
{
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

class ShellCorona : public Plasma::Corona, QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(QString shell READ shell WRITE setShell)
    Q_PROPERTY(int numScreens READ numScreens)
    Q_CLASSINFO("D-Bus Interface", "org.kde.PlasmaShell")

public:
    explicit ShellCorona(QObject *parent = nullptr);
    ~ShellCorona() override;

    KPackage::Package lookAndFeelPackage();
    void init();

    /**
     * Where to save global configuration that doesn't have anything to do with the scene (e.g. views)
     */
    KSharedConfig::Ptr applicationConfig();

    int numScreens() const override;
    Q_INVOKABLE QRect screenGeometry(int id) const override;
    Q_INVOKABLE QRegion availableScreenRegion(int id) const override;
    Q_INVOKABLE QRect availableScreenRect(int id) const override;

    // plasmashellCorona's value
    QRegion _availableScreenRegion(int id) const;
    QRect _availableScreenRect(int id) const;

    Q_INVOKABLE QStringList availableActivities() const;

    PanelView *panelView(Plasma::Containment *containment) const;

    // This one is a bit of an hack but are just for desktop scripting
    void insertActivity(const QString &id, const QString &plugin);

    Plasma::Containment *setContainmentTypeForScreen(int screen, const QString &plugin);

    void removeDesktop(DesktopView *desktopView);

    /**
     * @returns a new containment associated with the specified @p activity and @p screen.
     */
    Plasma::Containment *createContainmentForActivity(const QString &activity, int screenNum);

    KWayland::Client::PlasmaShell *waylandPlasmaShellInterface() const;

    ScreenPool *screenPool() const;

    QList<int> screenIds() const;

    QString defaultContainmentPlugin() const;

    static QString defaultShell();

Q_SIGNALS:
    void glInitializationFailed();

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

    /// DBUS methods
    void toggleDashboard();
    void setDashboardShown(bool show);
    void toggleActivityManager();
    void toggleWidgetExplorer();
    QString evaluateScript(const QString &string);
    void activateLauncherMenu();

    QByteArray dumpCurrentLayoutJS() const;

    /**
     * loads the shell layout from a look and feel package,
     * resetting it to the default layout exported in the
     * look and feel package
     */
    void loadLookAndFeelDefaultLayout(const QString &layout);

    Plasma::Containment *addPanel(const QString &plugin);

    void nextActivity();
    void previousActivity();
    void stopCurrentActivity();

    void setTestModeLayout(const QString &layout)
    {
        m_testModeLayout = layout;
    }

    int panelCount() const
    {
        return m_panelViews.count();
    }

    void refreshCurrentShell();

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
    void loadDefaultLayout() override;

    /**
     * Execute any update script
     */
    void processUpdateScripts();

    int screenForContainment(const Plasma::Containment *containment) const override;

    void showAlternativesForApplet(Plasma::Applet *applet);

private Q_SLOTS:
    void createWaitingPanels();
    void handleContainmentAdded(Plasma::Containment *c);
    void syncAppConfig();
    void checkActivities();
    void currentActivityChanged(const QString &newActivity);
    void activityAdded(const QString &id);
    void activityRemoved(const QString &id);
    void checkAddPanelAction();
    void addPanel();
    void addPanel(QAction *action);
    void populateAddPanelsMenu();

    void addOutput(QScreen *screen);
    void primaryScreenChanged(QScreen *oldScreen, QScreen *newScreen);

    void panelContainmentDestroyed(QObject *cont);
    void handleScreenRemoved(QScreen *screen);

    void activateTaskManagerEntry(int index);

private:
    void updateStruts();
    void configurationChanged(const QString &path);
    QList<PanelView *> panelsForScreen(QScreen *screen) const;
    DesktopView *desktopForScreen(QScreen *screen) const;
    void setupWaylandIntegration();
    void executeSetupPlasmoidScript(Plasma::Containment *containment, Plasma::Applet *applet);
    void checkAllDesktopsUiReady(bool ready);

#ifndef NDEBUG
    void screenInvariants() const;
#endif

    void insertContainment(const QString &activity, int screenNum, Plasma::Containment *containment);

    KSharedConfig::Ptr m_config;
    QString m_configPath;

    ScreenPool *m_screenPool;
    QString m_shell;
    KActivities::Controller *m_activityController;
    QHash<const Plasma::Containment *, PanelView *> m_panelViews;
    // map from QScreen to desktop view
    QHash<const QScreen *, DesktopView *> m_desktopViewForScreen;
    QHash<const Plasma::Containment *, int> m_pendingScreenChanges;
    KConfigGroup m_desktopDefaultsConfig;
    KConfigGroup m_lnfDefaultsConfig;
    QList<Plasma::Containment *> m_waitingPanels;
    QHash<QString, QString> m_activityContainmentPlugins;
    QAction *m_addPanelAction;
    QScopedPointer<QMenu> m_addPanelsMenu;
    KPackage::Package m_lookAndFeelPackage;

    QTimer m_waitingPanelsTimer;
    QTimer m_appConfigSyncTimer;
#ifndef NDEBUG
    QTimer m_invariantsTimer;
#endif
    KWayland::Client::PlasmaShell *m_waylandPlasmaShell;
    bool m_closingDown : 1;
    QString m_testModeLayout;

    StrutManager *m_strutManager;
};
