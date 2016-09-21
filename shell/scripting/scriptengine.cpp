/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
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

#include "scriptengine.h"
#include "scriptengine_v1.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QScriptValueIterator>
#include <QStandardPaths>
#include <QFutureWatcher>

#include <QDebug>
#include <klocalizedstring.h>
#include <kmimetypetrader.h>
#include <kservicetypetrader.h>
#include <kshell.h>

// KIO
//#include <kemailsettings.h> // no camelcase include

#include <Plasma/Applet>
#include <Plasma/PluginLoader>
#include <Plasma/Containment>
#include <qstandardpaths.h>
#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include "appinterface.h"
#include "containment.h"
#include "configgroup.h"
#include "i18n.h"
#include "panel.h"
#include "widget.h"
#include "../shellcorona.h"
#include "../standaloneappcorona.h"
#include "../screenpool.h"

QScriptValue constructQRectFClass(QScriptEngine *engine);

namespace WorkspaceScripting
{

ScriptEngine::ScriptEngine(Plasma::Corona *corona, QObject *parent)
    : QScriptEngine(parent),
      m_corona(corona)
{
    Q_ASSERT(m_corona);
    AppInterface *interface = new AppInterface(this);
    connect(interface, &AppInterface::print, this, &ScriptEngine::print);
    m_scriptSelf = newQObject(interface, QScriptEngine::QtOwnership,
                              QScriptEngine::ExcludeSuperClassProperties |
                              QScriptEngine::ExcludeSuperClassMethods);
    setupEngine();
    connect(this, &ScriptEngine::signalHandlerException, this, &ScriptEngine::exception);
    bindI18N(this);
}

ScriptEngine::~ScriptEngine()
{
}

QScriptValue ScriptEngine::wrap(Plasma::Applet *w)
{
    Widget *wrapper = new Widget(w);
    QScriptValue v = newQObject(wrapper, QScriptEngine::ScriptOwnership,
                                QScriptEngine::ExcludeSuperClassProperties |
                                QScriptEngine::ExcludeSuperClassMethods);
    return v;
}

QScriptValue ScriptEngine::wrap(Plasma::Containment *c)
{
    Containment *wrapper = isPanel(c) ? new Panel(c) : new Containment(c);
    return wrap(wrapper);
}

QScriptValue ScriptEngine::wrap(Containment *c)
{
    QScriptValue v = newQObject(c, QScriptEngine::ScriptOwnership,
                                QScriptEngine::ExcludeSuperClassProperties |
                                QScriptEngine::ExcludeSuperClassMethods);
    v.setProperty(QStringLiteral("widgetById"), newFunction(Containment::widgetById));
    v.setProperty(QStringLiteral("addWidget"), newFunction(Containment::addWidget));
    v.setProperty(QStringLiteral("widgets"), newFunction(Containment::widgets));

    return v;
}

int ScriptEngine::defaultPanelScreen() const
{
    return 0;
}

ScriptEngine *ScriptEngine::envFor(QScriptEngine *engine)
{
    QObject *object = engine->globalObject().toQObject();
    Q_ASSERT(object);

    AppInterface *interface = qobject_cast<AppInterface *>(object);
    Q_ASSERT(interface);

    ScriptEngine *env = qobject_cast<ScriptEngine *>(interface->parent());
    Q_ASSERT(env);

    return env;
}

QString ScriptEngine::onlyExec(const QString &commandLine)
{
    if (commandLine.isEmpty()) {
        return commandLine;
    }

    return KShell::splitArgs(commandLine, KShell::TildeExpand).first();
}

void ScriptEngine::setupEngine()
{
    QScriptValue v = globalObject();
    QScriptValueIterator it(v);
    while (it.hasNext()) {
        it.next();
        // we provide our own print implementation, but we want the rest
        if (it.name() != QLatin1String("print")) {
            m_scriptSelf.setProperty(it.name(), it.value());
        }
    }

    m_scriptSelf.setProperty(QStringLiteral("getApiVersion"), newFunction(ScriptEngine::createAPIForVersion));

    m_scriptSelf.setProperty(QStringLiteral("QRectF"), constructQRectFClass(this));
    m_scriptSelf.setProperty(QStringLiteral("createActivity"), newFunction(ScriptEngine::V1::createActivity));
    m_scriptSelf.setProperty(QStringLiteral("setCurrentActivity"), newFunction(ScriptEngine::V1::setCurrentActivity));
    m_scriptSelf.setProperty(QStringLiteral("currentActivity"), newFunction(ScriptEngine::V1::currentActivity));
    m_scriptSelf.setProperty(QStringLiteral("activities"), newFunction(ScriptEngine::V1::activities));
    m_scriptSelf.setProperty(QStringLiteral("activityName"), newFunction(ScriptEngine::V1::activityName));
    m_scriptSelf.setProperty(QStringLiteral("setActivityName"), newFunction(ScriptEngine::V1::setActivityName));
    m_scriptSelf.setProperty(QStringLiteral("loadSerializedLayout"), newFunction(ScriptEngine::V1::loadSerializedLayout));
    m_scriptSelf.setProperty(QStringLiteral("Panel"), newFunction(ScriptEngine::V1::newPanel, newObject()));
    m_scriptSelf.setProperty(QStringLiteral("desktopsForActivity"), newFunction(ScriptEngine::V1::desktopsForActivity));
    m_scriptSelf.setProperty(QStringLiteral("desktops"), newFunction(ScriptEngine::V1::desktops));
    m_scriptSelf.setProperty(QStringLiteral("desktopById"), newFunction(ScriptEngine::V1::desktopById));
    m_scriptSelf.setProperty(QStringLiteral("desktopForScreen"), newFunction(ScriptEngine::V1::desktopForScreen));
    m_scriptSelf.setProperty(QStringLiteral("panelById"), newFunction(ScriptEngine::V1::panelById));
    m_scriptSelf.setProperty(QStringLiteral("panels"), newFunction(ScriptEngine::V1::panels));
    m_scriptSelf.setProperty(QStringLiteral("fileExists"), newFunction(ScriptEngine::V1::fileExists));
    m_scriptSelf.setProperty(QStringLiteral("loadTemplate"), newFunction(ScriptEngine::V1::loadTemplate));
    m_scriptSelf.setProperty(QStringLiteral("applicationExists"), newFunction(ScriptEngine::V1::applicationExists));
    m_scriptSelf.setProperty(QStringLiteral("defaultApplication"), newFunction(ScriptEngine::V1::defaultApplication));
    m_scriptSelf.setProperty(QStringLiteral("userDataPath"), newFunction(ScriptEngine::V1::userDataPath));
    m_scriptSelf.setProperty(QStringLiteral("applicationPath"), newFunction(ScriptEngine::V1::applicationPath));
    m_scriptSelf.setProperty(QStringLiteral("knownWallpaperPlugins"), newFunction(ScriptEngine::V1::knownWallpaperPlugins));
    m_scriptSelf.setProperty(QStringLiteral("ConfigFile"), newFunction(ScriptEngine::V1::configFile));
    m_scriptSelf.setProperty(QStringLiteral("gridUnit"), ScriptEngine::V1::gridUnit());

    setGlobalObject(m_scriptSelf);
}

QScriptValue ScriptEngine::createAPIForVersion(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() < 1) {
        return context->throwError(i18n("getApiVersion() needs you to specify the version"));
    }

    const uint version = context->argument(0).toInt32();

    if (version != 1) {
        return context->throwError(i18n("getApiVersion() invalid version number"));
    }

    return envFor(engine)->m_scriptSelf;
}

bool ScriptEngine::isPanel(const Plasma::Containment *c)
{
    if (!c) {
        return false;
    }

    return c->containmentType() == Plasma::Types::PanelContainment ||
           c->containmentType() == Plasma::Types::CustomPanelContainment;
}

Plasma::Corona *ScriptEngine::corona() const
{
    return m_corona;
}

bool ScriptEngine::evaluateScript(const QString &script, const QString &path)
{
    //qDebug() << "evaluating" << m_editor->toPlainText();
    evaluate(script, path);
    if (hasUncaughtException()) {
        //qDebug() << "catch the exception!";
        QString error = i18n("Error: %1 at line %2\n\nBacktrace:\n%3",
                             uncaughtException().toString(),
                             QString::number(uncaughtExceptionLineNumber()),
                             uncaughtExceptionBacktrace().join(QStringLiteral("\n  ")));
        emit printError(error);
        return false;
    }

    return true;
}

void ScriptEngine::exception(const QScriptValue &value)
{
    //qDebug() << "exception caught!" << value.toVariant();
    emit printError(value.toVariant().toString());
}

QStringList ScriptEngine::pendingUpdateScripts(Plasma::Corona *corona)
{
    if (!corona->package().metadata().isValid()) {
        qWarning() << "Warning: corona package invalid";
        return QStringList();
    }

    const QString appName = corona->package().metadata().pluginName();
    QStringList scripts;

    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "plasma/shells/" + appName + QStringLiteral("/contents/updates"), QStandardPaths::LocateDirectory);
    Q_FOREACH(const QString& dir, dirs) {
        QDirIterator it(dir, QStringList() << QStringLiteral("*.js"));
        while (it.hasNext()) {
            scripts.append(it.next());
        }
    }
    QStringList scriptPaths;

    if (scripts.isEmpty()) {
        //qDebug() << "no update scripts";
        return scriptPaths;
    }

    KConfigGroup cg(KSharedConfig::openConfig(), "Updates");
    QStringList performed = cg.readEntry("performed", QStringList());
    const QString localXdgDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    foreach (const QString &script, scripts) {
        if (performed.contains(script)) {
            continue;
        }

        if (script.startsWith(localXdgDir)) {
            // qDebug() << "skipping user local script: " << script;
            continue;
        }

        scriptPaths.append(script);
        performed.append(script);
    }

    cg.writeEntry("performed", performed);
    KSharedConfig::openConfig()->sync();
    return scriptPaths;
}

QStringList ScriptEngine::availableActivities() const
{
    ShellCorona *sc = qobject_cast<ShellCorona *>(m_corona);
    StandaloneAppCorona *ac = qobject_cast<StandaloneAppCorona *>(m_corona);
    if (sc) {
        return sc->availableActivities();
    } else if (ac) {
        return ac->availableActivities();
    }

    return QStringList();
}

QList<Containment*> ScriptEngine::desktopsForActivity(const QString &id)
{
    QList<Containment*> result;

    // confirm this activity actually exists
    bool found = false;
    for (const QString &act: availableActivities()) {
        if (act == id) {
            found = true;
            break;
        }
    }

    if (!found) {
        return result;
    }

    foreach (Plasma::Containment *c, m_corona->containments()) {
        if (c->activity() == id && !isPanel(c)) {
            result << new Containment(c);
        }
    }

    if (result.count() == 0) {
        // we have no desktops for this activity, so lets make them now
        // this can happen when the activity already exists but has never been activated
        // with the current shell package and layout.js is run to set up the shell for the
        // first time
        ShellCorona *sc = qobject_cast<ShellCorona *>(m_corona);
        StandaloneAppCorona *ac = qobject_cast<StandaloneAppCorona *>(m_corona);
        if (sc) {
            foreach (int i, sc->screenIds()) {
                result << new Containment(sc->createContainmentForActivity(id, i));
            }
        } else if (ac) {
            const int numScreens = m_corona->numScreens();
            for (int i = 0; i < numScreens; ++i) {
                result << new Containment(ac->createContainmentForActivity(id, i));
            }
        }
    }

    return result;
}

int ScriptEngine::gridUnit()
{
    int gridUnit = QFontMetrics(QGuiApplication::font()).boundingRect(QStringLiteral("M")).height();
    if (gridUnit % 2 != 0) {
        gridUnit++;
    }

    return gridUnit;
}

Containment *ScriptEngine::createContainment(const QString &type, const QString &plugin)
{
    bool exists = false;
    const KPluginInfo::List list = Plasma::PluginLoader::listContainmentsOfType(type);
    foreach (const KPluginInfo &info, list) {
        if (info.pluginName() == plugin) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        return nullptr;
    }

    Plasma::Containment *c = 0;
    if (type == QLatin1String("Panel")) {
        ShellCorona *sc = qobject_cast<ShellCorona *>(m_corona);
        StandaloneAppCorona *ac = qobject_cast<StandaloneAppCorona *>(m_corona);
        if (sc) {
            c = sc->addPanel(plugin);
        } else if (ac) {
            c = ac->addPanel(plugin);
        }
    } else {
        c = m_corona->createContainment(plugin);
    }

    if (c) {
        if (type == QLatin1String("Panel")) {
            // some defaults
            c->setFormFactor(Plasma::Types::Horizontal);
            c->setLocation(Plasma::Types::TopEdge);
            //we have to force lastScreen of the newly created containment,
            //or it won't have a screen yet at that point, breaking JS code
            //that relies on it
            //NOTE: if we'll allow setting a panel screen from JS, it will have to use the following lines as well
            KConfigGroup cg=c->config();
            cg.writeEntry(QStringLiteral("lastScreen"), 0);
            c->restore(cg);
        }
        c->updateConstraints(Plasma::Types::AllConstraints | Plasma::Types::StartupCompletedConstraint);
        c->flushPendingConstraintsEvents();
    }

    return isPanel(c) ? new Panel(c) : new Containment(c);
}

} // namespace WorkspaceScripting


