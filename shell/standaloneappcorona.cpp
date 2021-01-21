/*
 * Copyright 2014  Bhushan Shah <bhush94@gmail.com>
 * Copyright 2014 Marco Martin <notmart@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "standaloneappcorona.h"
#include "panelview.h"

#include <QAction>
#include <QDebug>
#include <QQuickItem>

#include <KActionCollection>
#include <Plasma/PluginLoader>
#include <kactivities/consumer.h>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include "scripting/scriptengine.h"

StandaloneAppCorona::StandaloneAppCorona(const QString &coronaPlugin, QObject *parent)
    : Plasma::Corona(parent)
    , m_coronaPlugin(coronaPlugin)
    , m_activityConsumer(new KActivities::Consumer(this))
    , m_view(nullptr)
{
    qmlRegisterUncreatableType<DesktopView>("org.kde.plasma.shell", 2, 0, "Desktop", QStringLiteral("It is not possible to create objects of type Desktop"));
    qmlRegisterUncreatableType<PanelView>("org.kde.plasma.shell", 2, 0, "Panel", QStringLiteral("It is not possible to create objects of type Panel"));

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Shell"));
    package.setPath(m_coronaPlugin);
    package.setAllowExternalPaths(true);
    setKPackage(package);

    Plasma::Theme theme;
    theme.setUseGlobalSettings(false);

    KConfigGroup lnfCfg = KConfigGroup(KSharedConfig::openConfig(package.filePath("defaults")), "Theme");
    theme.setThemeName(lnfCfg.readEntry("name", "default"));

    m_desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package.filePath("defaults")), "Desktop");

    m_view = new DesktopView(this);

    connect(m_activityConsumer, &KActivities::Consumer::currentActivityChanged, this, &StandaloneAppCorona::currentActivityChanged);
    connect(m_activityConsumer, &KActivities::Consumer::activityAdded, this, &StandaloneAppCorona::activityAdded);
    connect(m_activityConsumer, &KActivities::Consumer::activityRemoved, this, &StandaloneAppCorona::activityRemoved);

    connect(m_activityConsumer, &KActivities::Consumer::serviceStatusChanged, this, &StandaloneAppCorona::load);
}

StandaloneAppCorona::~StandaloneAppCorona()
{
    delete m_view;
}

QRect StandaloneAppCorona::screenGeometry(int id) const
{
    Q_UNUSED(id);
    if (m_view) {
        return m_view->geometry();
    } else {
        return QRect();
    }
}

void StandaloneAppCorona::load()
{
    loadLayout("plasma-" + m_coronaPlugin + "-appletsrc");

    bool found = false;
    for (auto c : containments()) {
        if (c->containmentType() == Plasma::Types::DesktopContainment || c->containmentType() == Plasma::Types::CustomContainment) {
            found = true;
            break;
        }
    }

    if (!found) {
        qDebug() << "Loading default layout";
        loadDefaultLayout();
        saveLayout("plasma-" + m_coronaPlugin + "-appletsrc");
    }

    for (auto c : containments()) {
        qDebug() << "containment found";
        if (c->containmentType() == Plasma::Types::DesktopContainment || c->containmentType() == Plasma::Types::CustomContainment) {
            QAction *removeAction = c->actions()->action(QStringLiteral("remove"));
            if (removeAction) {
                removeAction->deleteLater();
            }
            m_view->setContainment(c);
            m_view->show();
            connect(m_view, &QWindow::visibleChanged, [=](bool visible) {
                if (!visible) {
                    deleteLater();
                }
            });
            break;
        }
    }
}

void StandaloneAppCorona::loadDefaultLayout()
{
    const QString script = kPackage().filePath("defaultlayout");
    QFile file(script);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString code = file.readAll();
        qDebug() << "evaluating startup script:" << script;

        WorkspaceScripting::ScriptEngine scriptEngine(this);

        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this, [](const QString &msg) {
            qWarning() << msg;
        });
        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this, [](const QString &msg) {
            qDebug() << msg;
        });
        scriptEngine.evaluateScript(code);
    }
}

Plasma::Containment *StandaloneAppCorona::createContainmentForActivity(const QString &activity, int screenNum)
{
    for (Plasma::Containment *cont : containments()) {
        if (cont->activity() == activity
            && (cont->containmentType() == Plasma::Types::DesktopContainment || cont->containmentType() == Plasma::Types::CustomContainment)) {
            return cont;
        }
    }

    Plasma::Containment *containment =
        containmentForScreen(screenNum, m_desktopDefaultsConfig.readEntry("Containment", "org.kde.desktopcontainment"), QVariantList());
    Q_ASSERT(containment);

    if (containment) {
        containment->setActivity(activity);
    }

    return containment;
}

void StandaloneAppCorona::activityAdded(const QString &id)
{
    // TODO more sanity checks
    if (m_activityContainmentPlugins.contains(id)) {
        qWarning() << "Activity added twice" << id;
        return;
    }

    m_activityContainmentPlugins.insert(id, QString());
}

void StandaloneAppCorona::activityRemoved(const QString &id)
{
    m_activityContainmentPlugins.remove(id);
}

void StandaloneAppCorona::currentActivityChanged(const QString &newActivity)
{
    // qDebug() << "Activity changed:" << newActivity;

    if (containments().isEmpty()) {
        return;
    }

    Plasma::Containment *c = createContainmentForActivity(newActivity, 0);

    connect(c, &Plasma::Containment::showAddWidgetsInterface, this, &StandaloneAppCorona::toggleWidgetExplorer);

    QAction *removeAction = c->actions()->action(QStringLiteral("remove"));
    if (removeAction) {
        removeAction->deleteLater();
    }
    m_view->setContainment(c);
}

void StandaloneAppCorona::toggleWidgetExplorer()
{
    // The view QML has to provide something to display the widget explorer
    m_view->rootObject()->metaObject()->invokeMethod(m_view->rootObject(), "toggleWidgetExplorer", Q_ARG(QVariant, QVariant::fromValue(sender())));
    return;
}

QStringList StandaloneAppCorona::availableActivities() const
{
    return m_activityContainmentPlugins.keys();
}

void StandaloneAppCorona::insertActivity(const QString &id, const QString &plugin)
{
    m_activityContainmentPlugins.insert(id, plugin);
    Plasma::Containment *c = createContainmentForActivity(id, 0);
    if (c) {
        c->config().writeEntry("lastScreen", 0);
    }
}

Plasma::Containment *StandaloneAppCorona::addPanel(const QString &plugin)
{
    // this creates a panel that wwill be used for nothing
    // it's needed by the scriptengine to create
    // a corona useful also when launched in fullshell
    Plasma::Containment *panel = createContainment(plugin);
    if (!panel) {
        return nullptr;
    }

    return panel;
}

int StandaloneAppCorona::screenForContainment(const Plasma::Containment *containment) const
{
    // this simple corona doesn't have multiscreen support
    if (containment->containmentType() != Plasma::Types::PanelContainment && containment->containmentType() != Plasma::Types::CustomPanelContainment) {
        if (containment->activity() != m_activityConsumer->currentActivity()) {
            return -1;
        }
    }
    return 0;
}
