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
#include <QFutureWatcher>
#include <QJSValueIterator>
#include <QStandardPaths>

#include <KLocalizedContext>
#include <QDebug>
#include <klocalizedstring.h>
#include <kmimetypetrader.h>
#include <kservicetypetrader.h>
#include <kshell.h>

#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/PluginLoader>
#include <qstandardpaths.h>

#include "../screenpool.h"
#include "../standaloneappcorona.h"
#include "appinterface.h"
#include "configgroup.h"
#include "containment.h"
#include "panel.h"
#include "widget.h"

namespace WorkspaceScripting
{
ScriptEngine::ScriptEngine(Plasma::Corona *corona, QObject *parent)
    : QJSEngine(parent)
    , m_corona(corona)
{
    Q_ASSERT(m_corona);
    m_appInterface = new AppInterface(this);
    connect(m_appInterface, &AppInterface::print, this, &ScriptEngine::print);
    m_scriptSelf = globalObject();
    m_globalScriptEngineObject = new ScriptEngine::V1(this);
    m_localizedContext = new KLocalizedContext(this);
    setupEngine();
}

ScriptEngine::~ScriptEngine()
{
}

QString ScriptEngine::errorString() const
{
    return m_errorString;
}

QJSValue ScriptEngine::wrap(Plasma::Applet *w)
{
    Widget *wrapper = new Widget(w, this);
    return newQObject(wrapper);
}

QJSValue ScriptEngine::wrap(Plasma::Containment *c)
{
    Containment *wrapper = isPanel(c) ? new Panel(c, this) : new Containment(c, this);
    return newQObject(wrapper);
}

int ScriptEngine::defaultPanelScreen() const
{
    return 1;
}

QJSValue ScriptEngine::newError(const QString &message)
{
    return evaluate(QStringLiteral("new Error('%1');").arg(message));
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
    QJSValue globalScriptEngineObject = newQObject(m_globalScriptEngineObject);
    QJSValue localizedContext = newQObject(m_localizedContext);
    QJSValue appInterface = newQObject(m_appInterface);

    // AppInterface stuff
    // FIXME: this line doesn't have much effect for now, if QTBUG-68397 gets fixed,
    // all the connects to rewrite the properties won't be necessary anymore
    // globalObject().setPrototype(appInterface);
    // FIXME: remove __AppInterface if QTBUG-68397 gets solved
    // as workaround we build manually a js object with getters and setters
    m_scriptSelf.setProperty(QStringLiteral("__AppInterface"), appInterface);
    QJSValue res = evaluate(
        "__proto__ = {\
                get locked() {return __AppInterface.locked;},\
                get hasBattery() {return __AppInterface.hasBattery;},\
                get screenCount() {return __AppInterface.screenCount;},\
                get activityIds() {return __AppInterface.activityIds;},\
                get panelIds() {return __AppInterface.panelIds;},\
                get knownPanelTypes() {return __AppInterface.knownPanelTypes;},\
                get knownActivityTypes() {return __AppInterface.knownActivityTypes;},\
                get knownWidgetTypes() {return __AppInterface.knownWidgetTypes;},\
                get theme() {return __AppInterface.theme;},\
                set theme(name) {__AppInterface.theme = name;},\
                get applicationVersion() {return __AppInterface.applicationVersion;},\
                get platformVersion() {return __AppInterface.platformVersion;},\
                get scriptingVersion() {return __AppInterface.scriptingVersion;},\
                get multihead() {return __AppInterface.multihead;},\
                get multiheadScreen() {return __AppInterface.multihead;},\
                get locale() {return __AppInterface.locale;},\
                get language() {return __AppInterface.language;},\
                get languageId() {return __AppInterface.languageId;},\
            }");
    Q_ASSERT(!res.isError());
    // methods from AppInterface
    m_scriptSelf.setProperty(QStringLiteral("screenGeometry"), appInterface.property("screenGeometry"));
    m_scriptSelf.setProperty(QStringLiteral("lockCorona"), appInterface.property("lockCorona"));
    m_scriptSelf.setProperty(QStringLiteral("sleep"), appInterface.property("sleep"));
    m_scriptSelf.setProperty(QStringLiteral("print"), appInterface.property("print"));

    m_scriptSelf.setProperty(QStringLiteral("getApiVersion"), globalScriptEngineObject.property("getApiVersion"));

    // Constructors: prefer them js based as they make the c++ code of panel et al way simpler without hacks to get the engine
    m_scriptSelf.setProperty(QStringLiteral("__newPanel"), globalScriptEngineObject.property("newPanel"));
    m_scriptSelf.setProperty(QStringLiteral("__newConfigFile"), globalScriptEngineObject.property("configFile"));
    // definitions of qrectf properties from documentation
    // only properties/functions which were already binded are.
    // TODO KF6: just a plain QRectF binding
    res = evaluate(
        "function QRectF(x,y,w,h) {\
                return {x: x, y: y, width: w, height: h,\
                        get left() {return this.x},\
                        get top() {return this.y},\
                        get right() {return this.x + this.width},\
                        get bottom() {return this.y + this.height},\
                        get empty() {return this.width <= 0 || this.height <= 0},\
                        get null() {return this.width == 0 || this.height == 0},\
                        get valid() {return !this.empty},\
                        adjust: function(dx1, dy1, dx2, dy2) {\
                            this.x += dx1; this.y += dy1;\
                            this.width = this.width - dx1 + dx2;\
                            this.height = this.height - dy1 + dy2;},\
                        adjusted: function(dx1, dy1, dx2, dy2) {\
                            return new QRectF(this.x + dx1, this.y + dy1,\
                                              this.width - dx1 + dx2,\
                                              this.height - dy1 + dy2)},\
                        translate: function(dx, dy) {this.x += dx; this.y += dy;},\
                        setCoords: function(x1, y1, x2, y2) {\
                            this.x = x1; this.y = y1;\
                            this.width = x2 - x1;\
                            this.height = y2 - y1;},\
                        setRect: function(x1, y1, w1, h1) {\
                            this.x = x1; this.y = y1;\
                            this.width = w1; this.height = h1;},\
                        contains: function(x1, y1) { return x1 >= this.x && x1 <= this.x + this.width && y1 >= this.y && y1 <= this.y + this.height},\
                        moveBottom: function(bottom1) {this.y = bottom1 - this.height;},\
                        moveLeft: function(left1) {this.x = left1;},\
                        moveRight: function(right1) {this.x = right1 - this.width;},\
                        moveTop: function(top1) {this.y = top1;},\
                        moveTo: function(x1, y1) {this.x = x1; this.y = y1;}\
              }};\
              function ConfigFile(config, group){return __newConfigFile(config, group)};\
              function Panel(plugin){return __newPanel(plugin)};");
    Q_ASSERT(!res.isError());

    m_scriptSelf.setProperty(QStringLiteral("createActivity"), globalScriptEngineObject.property("createActivity"));
    m_scriptSelf.setProperty(QStringLiteral("setCurrentActivity"), globalScriptEngineObject.property("setCurrentActivity"));
    m_scriptSelf.setProperty(QStringLiteral("currentActivity"), globalScriptEngineObject.property("currentActivity"));
    m_scriptSelf.setProperty(QStringLiteral("activities"), globalScriptEngineObject.property("activities"));
    m_scriptSelf.setProperty(QStringLiteral("activityName"), globalScriptEngineObject.property("activityName"));
    m_scriptSelf.setProperty(QStringLiteral("setActivityName"), globalScriptEngineObject.property("setActivityName"));
    m_scriptSelf.setProperty(QStringLiteral("loadSerializedLayout"), globalScriptEngineObject.property("loadSerializedLayout"));
    m_scriptSelf.setProperty(QStringLiteral("desktopsForActivity"), globalScriptEngineObject.property("desktopsForActivity"));
    m_scriptSelf.setProperty(QStringLiteral("desktops"), globalScriptEngineObject.property("desktops"));
    m_scriptSelf.setProperty(QStringLiteral("desktopById"), globalScriptEngineObject.property("desktopById"));
    m_scriptSelf.setProperty(QStringLiteral("desktopForScreen"), globalScriptEngineObject.property("desktopForScreen"));
    m_scriptSelf.setProperty(QStringLiteral("screenForConnector"), globalScriptEngineObject.property("screenForConnector"));
    m_scriptSelf.setProperty(QStringLiteral("panelById"), globalScriptEngineObject.property("panelById"));
    m_scriptSelf.setProperty(QStringLiteral("panels"), globalScriptEngineObject.property("panels"));
    m_scriptSelf.setProperty(QStringLiteral("fileExists"), globalScriptEngineObject.property("fileExists"));
    m_scriptSelf.setProperty(QStringLiteral("loadTemplate"), globalScriptEngineObject.property("loadTemplate"));
    m_scriptSelf.setProperty(QStringLiteral("applicationExists"), globalScriptEngineObject.property("applicationExists"));
    m_scriptSelf.setProperty(QStringLiteral("defaultApplication"), globalScriptEngineObject.property("defaultApplication"));
    m_scriptSelf.setProperty(QStringLiteral("userDataPath"), globalScriptEngineObject.property("userDataPath"));
    m_scriptSelf.setProperty(QStringLiteral("applicationPath"), globalScriptEngineObject.property("applicationPath"));
    m_scriptSelf.setProperty(QStringLiteral("knownWallpaperPlugins"), globalScriptEngineObject.property("knownWallpaperPlugins"));
    m_scriptSelf.setProperty(QStringLiteral("gridUnit"), globalScriptEngineObject.property("gridUnit"));
    m_scriptSelf.setProperty(QStringLiteral("setImmutability"), globalScriptEngineObject.property("setImmutability"));
    m_scriptSelf.setProperty(QStringLiteral("immutability"), globalScriptEngineObject.property("immutability"));

    // i18n
    m_scriptSelf.setProperty(QStringLiteral("i18n"), localizedContext.property("i18n"));
    m_scriptSelf.setProperty(QStringLiteral("i18nc"), localizedContext.property("i18nc"));
    m_scriptSelf.setProperty(QStringLiteral("i18np"), localizedContext.property("i18np"));
    m_scriptSelf.setProperty(QStringLiteral("i18ncp"), localizedContext.property("i18ncp"));
    m_scriptSelf.setProperty(QStringLiteral("i18nd"), localizedContext.property("i18nd"));
    m_scriptSelf.setProperty(QStringLiteral("i18ndc"), localizedContext.property("i18ndc"));
    m_scriptSelf.setProperty(QStringLiteral("i18ndp"), localizedContext.property("i18ndp"));
    m_scriptSelf.setProperty(QStringLiteral("i18ndcp"), localizedContext.property("i18ndcp"));

    m_scriptSelf.setProperty(QStringLiteral("xi18n"), localizedContext.property("xi18n"));
    m_scriptSelf.setProperty(QStringLiteral("xi18nc"), localizedContext.property("xi18nc"));
    m_scriptSelf.setProperty(QStringLiteral("xi18np"), localizedContext.property("xi18np"));
    m_scriptSelf.setProperty(QStringLiteral("xi18ncp"), localizedContext.property("xi18ncp"));
    m_scriptSelf.setProperty(QStringLiteral("xi18nd"), localizedContext.property("xi18nd"));
    m_scriptSelf.setProperty(QStringLiteral("xi18ndc"), localizedContext.property("xi18ndc"));
    m_scriptSelf.setProperty(QStringLiteral("xi18ndp"), localizedContext.property("xi18ndp"));
    m_scriptSelf.setProperty(QStringLiteral("xi18ndcp"), localizedContext.property("xi18ndcp"));
}

bool ScriptEngine::isPanel(const Plasma::Containment *c)
{
    if (!c) {
        return false;
    }

    return c->containmentType() == Plasma::Types::PanelContainment || c->containmentType() == Plasma::Types::CustomPanelContainment;
}

Plasma::Corona *ScriptEngine::corona() const
{
    return m_corona;
}

bool ScriptEngine::evaluateScript(const QString &script, const QString &path)
{
    m_errorString = QString();

    QJSValue result = evaluate(script, path);
    if (result.isError()) {
        QString error = i18n("Error: %1 at line %2\n\nBacktrace:\n%3",
                             result.toString(),
                             result.property("lineNumber").toInt(),
                             result.property("stack").toVariant().value<QStringList>().join(QLatin1String("\n  ")));
        emit printError(error);
        emit exception(result);
        m_errorString = error;
        return false;
    }

    return true;
}

void ScriptEngine::exception(const QJSValue &value)
{
    emit printError(value.toVariant().toString());
}

QStringList ScriptEngine::pendingUpdateScripts(Plasma::Corona *corona)
{
    if (!corona->kPackage().isValid()) {
        qWarning() << "Warning: corona package invalid";
        return QStringList();
    }

    const QString appName = corona->kPackage().metadata().pluginId();
    QStringList scripts;

    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                       "plasma/shells/" + appName + QStringLiteral("/contents/updates"),
                                                       QStandardPaths::LocateDirectory);
    for (const QString &dir : dirs) {
        QDirIterator it(dir, QStringList() << QStringLiteral("*.js"));
        while (it.hasNext()) {
            scripts.append(it.next());
        }
    }
    QStringList scriptPaths;

    if (scripts.isEmpty()) {
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

QList<Containment *> ScriptEngine::desktopsForActivity(const QString &id)
{
    QList<Containment *> result;

    // confirm this activity actually exists
    bool found = false;
    for (const QString &act : availableActivities()) {
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
            result << new Containment(c, this);
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
                result << new Containment(sc->createContainmentForActivity(id, i), this);
            }
        } else if (ac) {
            const int numScreens = m_corona->numScreens();
            for (int i = 0; i < numScreens; ++i) {
                result << new Containment(ac->createContainmentForActivity(id, i), this);
            }
        }
    }

    return result;
}

Plasma::Containment *ScriptEngine::createContainment(const QString &type, const QString &plugin)
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

    Plasma::Containment *c = nullptr;
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
            // we have to force lastScreen of the newly created containment,
            // or it won't have a screen yet at that point, breaking JS code
            // that relies on it
            // NOTE: if we'll allow setting a panel screen from JS, it will have to use the following lines as well
            KConfigGroup cg = c->config();
            cg.writeEntry(QStringLiteral("lastScreen"), 0);
            c->restore(cg);
        }
        c->updateConstraints(Plasma::Types::AllConstraints | Plasma::Types::StartupCompletedConstraint);
        c->flushPendingConstraintsEvents();
    }

    return c;
}

Containment *ScriptEngine::createContainmentWrapper(const QString &type, const QString &plugin)
{
    Plasma::Containment *c = createContainment(type, plugin);
    return isPanel(c) ? new Panel(c, this) : new Containment(c, this);
}

} // namespace WorkspaceScripting
