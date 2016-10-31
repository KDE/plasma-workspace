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

namespace {
    template <typename T>
    inline void awaitFuture(const QFuture<T> &future)
    {
        while (!future.isFinished()) {
            QCoreApplication::processEvents();
        }
    }

    class ScriptArray_forEach_Helper {
    public:
        ScriptArray_forEach_Helper(const QScriptValue &array)
            : array(array)
        {
        }

        // operator + is commonly used for these things
        // to avoid having the lambda inside the parenthesis
        template <typename Function>
        void operator+ (Function function) const
        {
            if (!array.isArray()) return;

            int length = array.property("length").toInteger();
            for (int i = 0; i < length; ++i) {
                function(array.property(i));
            }
        }

    private:
        const QScriptValue &array;
    };

    #define SCRIPT_ARRAY_FOREACH(Variable, Array) \
        ScriptArray_forEach_Helper(Array) + [&] (const QScriptValue &Variable)

    class ScriptObject_forEach_Helper {
    public:
        ScriptObject_forEach_Helper(const QScriptValue &object)
            : object(object)
        {
        }

        // operator + is commonly used for these things
        // to avoid having the lambda inside the parenthesis
        template <typename Function>
        void operator+ (Function function) const
        {
            QScriptValueIterator it(object);
            while (it.hasNext()) {
                it.next();
                function(it.name(), it.value());
            }
        }

    private:
        const QScriptValue &object;
    };

    #define SCRIPT_OBJECT_FOREACH(Key, Value, Array) \
        ScriptObject_forEach_Helper(Array) + [&] (const QString &Key, const QScriptValue &Value)

    // Case insensitive comparison of two strings
    template <typename StringType>
    inline bool matches(const QString &object, const StringType &string)
    {
        return object.compare(string, Qt::CaseInsensitive) == 0;
    }
}

namespace WorkspaceScripting
{

QScriptValue ScriptEngine::V1::desktopById(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("activityById requires an id"));
    }

    const uint id = context->argument(0).toInt32();
    ScriptEngine *env = envFor(engine);
    foreach (Plasma::Containment *c, env->m_corona->containments()) {
        if (c->id() == id && !isPanel(c)) {
            return env->wrap(c);
        }
    }

    return engine->undefinedValue();
}

QScriptValue ScriptEngine::V1::desktopsForActivity(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("desktopsForActivity requires an id"));
    }

    QScriptValue containments = engine->newArray();
    int count = 0;

    const QString id = context->argument(0).toString();

    ScriptEngine *env = envFor(engine);

    const auto result = env->desktopsForActivity(id);

    for (Containment* c: result) {
        containments.setProperty(count, env->wrap(c));
        ++count;
    }

    containments.setProperty(QStringLiteral("length"), count);
    return containments;
}

QScriptValue ScriptEngine::V1::desktopForScreen(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("activityForScreen requires a screen id"));
    }

    const uint screen = context->argument(0).toInt32();
    ScriptEngine *env = envFor(engine);
    return env->wrap(env->m_corona->containmentForScreen(screen));
}

QScriptValue ScriptEngine::V1::createActivity(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() < 0) {
        return context->throwError(i18n("createActivity required the activity name"));
    }

    const QString name = context->argument(0).toString();
    QString plugin = context->argument(1).toString();

    ScriptEngine *env = envFor(engine);

    KActivities::Controller controller;

    // This is not the nicest way to do this, but createActivity
    // is a synchronous API :/
    QFuture<QString> futureId = controller.addActivity(name);
    awaitFuture(futureId);

    QString id = futureId.result();

    qDebug() << "Setting default Containment plugin:" << plugin;

    ShellCorona *sc = qobject_cast<ShellCorona *>(env->m_corona);
    StandaloneAppCorona *ac = qobject_cast<StandaloneAppCorona *>(env->m_corona);
    if (sc) {
        if (plugin.isEmpty() || plugin == QLatin1String("undefined")) {
            plugin = sc->defaultContainmentPlugin();
        }
        sc->insertActivity(id, plugin);
    } else if (ac) {
        if (plugin.isEmpty() || plugin == QLatin1String("undefined")) {
            KConfigGroup shellCfg = KConfigGroup(KSharedConfig::openConfig(env->m_corona->package().filePath("defaults")), "Desktop");
            plugin = shellCfg.readEntry("Containment", "org.kde.desktopcontainment");
        }
        ac->insertActivity(id, plugin);
    }

    return QScriptValue(id);
}

QScriptValue ScriptEngine::V1::setCurrentActivity(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() < 0) {
        return context->throwError(i18n("setCurrentActivity required the activity id"));
    }

    const QString id = context->argument(0).toString();

    KActivities::Controller controller;

    QFuture<bool> task = controller.setCurrentActivity(id);
    awaitFuture(task);

    return QScriptValue(task.result());
}

QScriptValue ScriptEngine::V1::setActivityName(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() < 2) {
        return context->throwError(i18n("setActivityName required the activity id and name"));
    }

    const QString id = context->argument(0).toString();
    const QString name = context->argument(1).toString();

    KActivities::Controller controller;

    QFuture<void> task = controller.setActivityName(id, name);
    awaitFuture(task);

    return QScriptValue();
}

QScriptValue ScriptEngine::V1::activityName(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() < 1) {
        return context->throwError(i18n("setActivityName required the activity id and name"));
    }

    const QString id = context->argument(0).toString();

    KActivities::Info info(id);

    return QScriptValue(info.name());
}

QScriptValue ScriptEngine::V1::currentActivity(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)
    Q_UNUSED(context)

    KActivities::Consumer consumer;
    return consumer.currentActivity();
}

QScriptValue ScriptEngine::V1::activities(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(context)

    return qScriptValueFromSequence(engine, envFor(engine)->availableActivities());
}

// Utility function to process configs and config groups
template <typename Object>
void loadSerializedConfigs(Object *object, const QScriptValue &configs)
{
    SCRIPT_OBJECT_FOREACH(escapedGroup, config, configs) {
        // If the config group is set, pass it on to the containment
        QStringList groups = escapedGroup.split('/', QString::SkipEmptyParts);
        for (QString &group: groups) {
            group = QUrl::fromPercentEncoding(group.toUtf8());
        }
        qDebug() << "Config group" << groups;
        object->setCurrentConfigGroup(groups);

        // Read other properties and set the configuration
        SCRIPT_OBJECT_FOREACH(key, value, config) {
            object->writeConfig(key, value.toVariant());
        };
    };
}

QScriptValue ScriptEngine::V1::loadSerializedLayout(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() < 1) {
        return context->throwError(i18n("loadSerializedLayout requires the JSON object to deserialize from"));
    }

    ScriptEngine *env = envFor(engine);

    const auto data = context->argument(0);

    if (data.property("serializationFormatVersion").toInteger() != 1) {
        return context->throwError(i18n("loadSerializedLayout: invalid version of the serialized object"));
    }

    const auto desktops = env->desktopsForActivity(KActivities::Consumer().currentActivity());
    Q_ASSERT_X(desktops.size() != 0, "V1::loadSerializedLayout", "We need desktops");

    // qDebug() << "DESKTOP DESERIALIZATION: Loading desktops...";

    int count = 0;
    SCRIPT_ARRAY_FOREACH(desktopData, data.property("desktops")) {
        // If the template has more desktops than we do, ignore them
        if (count >= desktops.size()) return;

        auto desktop = desktops[count];
        // qDebug() << "DESKTOP DESERIALIZATION: var cont = desktopsArray[...]; " << count << " -> " << desktop;

        // Setting the wallpaper plugin because it is special
        desktop->setWallpaperPlugin(desktopData.property("wallpaperPlugin").toString());
        // qDebug() << "DESKTOP DESERIALIZATION: cont->setWallpaperPlugin(...) " << desktop->wallpaperPlugin();

        // Now, lets go through the configs
        loadSerializedConfigs(desktop, desktopData.property("config"));

        // After the config, we want to load the applets
        SCRIPT_ARRAY_FOREACH(appletData, desktopData.property("applets")) {
            // qDebug() << "DESKTOP DESERIALIZATION: Applet: " << appletData.toString();

            // TODO: It would be nicer to be able to call addWidget directly
            auto desktopObject = env->wrap(desktop);
            auto addAppletFunction = desktopObject.property("addWidget");
            QScriptValueList args {
                appletData.property("plugin"),
                appletData.property("geometry.x").toInteger()      * ScriptEngine::gridUnit(),
                appletData.property("geometry.y").toInteger()      * ScriptEngine::gridUnit(),
                appletData.property("geometry.width").toInteger()  * ScriptEngine::gridUnit(),
                appletData.property("geometry.height").toInteger() * ScriptEngine::gridUnit()
            };

            auto appletObject = addAppletFunction.call(desktopObject, args);

            if (auto applet = qobject_cast<Widget*>(appletObject.toQObject())) {
                // Now, lets go through the configs for the applet
                loadSerializedConfigs(applet, appletData.property("config"));
            }
        };

        count++;
    };

    // qDebug() << "PANEL DESERIALIZATION: Loading panels...";

    SCRIPT_ARRAY_FOREACH(panelData, data.property("panels")) {
        const auto panel = qobject_cast<Panel *>(env->createContainment(
            QStringLiteral("Panel"), QStringLiteral("org.kde.panel")));

        Q_ASSERT(panel);

        // Basic panel setup
        panel->setLocation(panelData.property("location").toString());
        panel->setHeight(panelData.property("height").toNumber() * ScriptEngine::gridUnit());
        panel->setMaximumLength(panelData.property("maximumLength").toNumber() * ScriptEngine::gridUnit());
        panel->setMinimumLength(panelData.property("minimumLength").toNumber() * ScriptEngine::gridUnit());
        panel->setOffset(panelData.property("offset").toNumber() * ScriptEngine::gridUnit());
        panel->setAlignment(panelData.property("alignment").toString());

        // Loading the config for the panel
        loadSerializedConfigs(panel, panelData.property("config"));

        // Now dealing with the applets
        SCRIPT_ARRAY_FOREACH(appletData, panelData.property("applets")) {
            // qDebug() << "PANEL DESERIALIZATION: Applet: " << appletData.toString();

            // TODO: It would be nicer to be able to call addWidget directly
            auto panelObject = env->wrap(panel);
            auto addAppletFunction = panelObject.property("addWidget");
            QScriptValueList args { appletData.property("plugin") };

            auto appletObject = addAppletFunction.call(panelObject, args);
            // qDebug() << "PANEL DESERIALIZATION: addWidget"
            //      << appletData.property("plugin").toString()
            //      ;

            if (auto applet = qobject_cast<Widget*>(appletObject.toQObject())) {
                // Now, lets go through the configs for the applet
                loadSerializedConfigs(applet, appletData.property("config"));
            }
        };
    };

    return QScriptValue();
}

QScriptValue ScriptEngine::V1::newPanel(QScriptContext *context, QScriptEngine *engine)
{
    QString plugin(QStringLiteral("org.kde.panel"));

    if (context->argumentCount() > 0) {
        plugin = context->argument(0).toString();
    }

    return createContainment(QStringLiteral("Panel"), plugin, context, engine);
}

QScriptValue ScriptEngine::V1::panelById(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("panelById requires an id"));
    }

    const uint id = context->argument(0).toInt32();
    ScriptEngine *env = envFor(engine);
    foreach (Plasma::Containment *c, env->m_corona->containments()) {
        if (c->id() == id && isPanel(c)) {
            return env->wrap(c);
        }
    }

    return engine->undefinedValue();
}

QScriptValue ScriptEngine::V1::panels(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(context)

    QScriptValue panels = engine->newArray();
    ScriptEngine *env = envFor(engine);
    int count = 0;

    foreach (Plasma::Containment *c, env->m_corona->containments()) {
        if (isPanel(c)) {
            panels.setProperty(count, env->wrap(c));
            ++count;
        }
    }

    panels.setProperty(QStringLiteral("length"), count);
    return panels;
}

QScriptValue ScriptEngine::V1::fileExists(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)
    if (context->argumentCount() == 0) {
        return false;
    }

    const QString path = context->argument(0).toString();
    if (path.isEmpty()) {
        return false;
    }

    QFile f(KShell::tildeExpand(path));
    return f.exists();
}

QScriptValue ScriptEngine::V1::loadTemplate(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() == 0) {
        // qDebug() << "no arguments";
        return false;
    }

    const QString layout = context->argument(0).toString();
    if (layout.isEmpty() || layout.contains(QStringLiteral("'"))) {
        // qDebug() << "layout is empty";
        return false;
    }

    auto filter = [&layout](const KPluginMetaData &md) -> bool
    {
        return md.pluginId() == layout && md.value(QStringLiteral("X-Plasma-ContainmentCategories")).contains(QStringLiteral("panel"));
    };
    QList<KPluginMetaData> offers = KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/LayoutTemplate"), QString(), filter);

    if (offers.isEmpty()) {
        // qDebug() << "offers fail" << constraint;
        return false;
    }

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LayoutTemplate"));
    KPluginMetaData pluginData(offers.first());

    QString path;
    {
        ScriptEngine *env = envFor(engine);
        ShellCorona *sc = qobject_cast<ShellCorona *>(env->m_corona);
        if (sc) {
            const QString overridePackagePath = sc->lookAndFeelPackage().path() + QStringLiteral("contents/layouts/") + pluginData.pluginId();

            path = overridePackagePath + QStringLiteral("/metadata.json");
            if (QFile::exists(path)) {
                package.setPath(overridePackagePath);
            }

            path = overridePackagePath + QStringLiteral("/metadata.desktop");
            if (QFile::exists(path)) {
                package.setPath(overridePackagePath);
            }
        }
    }

    if (!package.isValid()) {
        path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, package.defaultPackageRoot() + pluginData.pluginId() + "/metadata.json");
        if (path.isEmpty()) {
            path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, package.defaultPackageRoot() + pluginData.pluginId() + "/metadata.desktop");
        }
        if (path.isEmpty()) {
            // qDebug() << "script path is empty";
            return false;
        }

        package.setPath(pluginData.pluginId());
    }

    const QString scriptFile = package.filePath("mainscript");
    if (scriptFile.isEmpty()) {
        // qDebug() << "scriptfile is empty";
        return false;
    }

    QFile file(scriptFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << i18n("Unable to load script file: %1", path);
        return false;
    }

    QString script = file.readAll();
    if (script.isEmpty()) {
        // qDebug() << "script is empty";
        return false;
    }

    ScriptEngine *env = envFor(engine);
    env->globalObject().setProperty(QStringLiteral("templateName"), env->newVariant(pluginData.name()), QScriptValue::ReadOnly | QScriptValue::Undeletable);
    env->globalObject().setProperty(QStringLiteral("templateComment"), env->newVariant(pluginData.description()), QScriptValue::ReadOnly | QScriptValue::Undeletable);

    QScriptValue rv = env->newObject();
    QScriptContext *ctx = env->pushContext();
    ctx->setThisObject(rv);

    env->evaluateScript(script, path);

    env->popContext();
    return rv;
}

QScriptValue ScriptEngine::V1::applicationExists(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)
    if (context->argumentCount() == 0) {
        return false;
    }

    const QString application = context->argument(0).toString();
    if (application.isEmpty()) {
        return false;
    }

    // first, check for it in $PATH
    if (!QStandardPaths::findExecutable(application).isEmpty()) {
        return true;
    }

    if (KService::serviceByStorageId(application)) {
        return true;
    }

    if (application.contains(QStringLiteral("'"))) {
        // apostrophes just screw up the trader lookups below, so check for it
        return false;
    }

    // next, consult ksycoca for an app by that name
    if (!KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("Name =~ '%1'").arg(application)).isEmpty()) {
        return true;
    }

    // next, consult ksycoca for an app by that generic name
    if (!KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("GenericName =~ '%1'").arg(application)).isEmpty()) {
        return true;
    }

    return false;
}

QScriptValue ScriptEngine::V1::defaultApplication(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() == 0) {
        return false;
    }

    const QString application = context->argument(0).toString();
    if (application.isEmpty()) {
        return false;
    }

    const bool storageId
        = context->argumentCount() < 2 ? false : context->argument(1).toBool();

    // FIXME: there are some pretty horrible hacks below, in the sense that they
    // assume a very
    // specific implementation system. there is much room for improvement here.
    // see
    // kdebase-runtime/kcontrol/componentchooser/ for all the gory details ;)
    if (matches(application, QLatin1String("mailer"))) {
        // KEMailSettings settings;

        // in KToolInvocation, the default is kmail; but let's be friendlier :)
        // QString command = settings.getSetting(KEMailSettings::ClientProgram);
        QString command;
        if (command.isEmpty()) {
            if (KService::Ptr kontact
                = KService::serviceByStorageId(QStringLiteral("kontact"))) {
                return storageId ? kontact->storageId()
                                 : onlyExec(kontact->exec());
            } else if (KService::Ptr kmail
                     = KService::serviceByStorageId(QStringLiteral("kmail"))) {
                return storageId ? kmail->storageId() : onlyExec(kmail->exec());
            }
        }

        if (!command.isEmpty()) {
            if (false) {
                KConfigGroup confGroup(KSharedConfig::openConfig(), "General");
                const QString preferredTerminal = confGroup.readPathEntry(
                    "TerminalApplication", QStringLiteral("konsole"));
                command = preferredTerminal + QLatin1String(" -e ") + command;
            }

            return command;
        }

    } else if (matches(application, QLatin1String("browser"))) {
        KConfigGroup config(KSharedConfig::openConfig(), "General");
        QString browserApp
            = config.readPathEntry("BrowserApplication", QString());
        if (browserApp.isEmpty()) {
            const KService::Ptr htmlApp
                = KMimeTypeTrader::self()->preferredService(QStringLiteral("text/html"));
            if (htmlApp) {
                browserApp = storageId ? htmlApp->storageId() : htmlApp->exec();
            }
        } else if (browserApp.startsWith('!')) {
            browserApp = browserApp.mid(1);
        }

        return onlyExec(browserApp);

    } else if (matches(application, QLatin1String("terminal"))) {
        KConfigGroup confGroup(KSharedConfig::openConfig(), "General");
        return onlyExec(confGroup.readPathEntry("TerminalApplication",
                                                QStringLiteral("konsole")));

    } else if (matches(application, QLatin1String("filemanager"))) {
        KService::Ptr service = KMimeTypeTrader::self()->preferredService(
            QStringLiteral("inode/directory"));
        if (service) {
            return storageId ? service->storageId() : onlyExec(service->exec());
        }

    } else if (matches(application, QLatin1String("windowmanager"))) {
        KConfig cfg(QStringLiteral("ksmserverrc"), KConfig::NoGlobals);
        KConfigGroup confGroup(&cfg, "General");
        return onlyExec(
            confGroup.readEntry("windowManager", QStringLiteral("kwin")));

    } else if (KService::Ptr service = KMimeTypeTrader::self()->preferredService(application)) {
        return storageId ? service->storageId() : onlyExec(service->exec());

    } else {
        // try the files in share/apps/kcm_componentchooser/
        const QStringList services = QStandardPaths::locateAll(
            QStandardPaths::GenericDataLocation,
            QStringLiteral("kcm_componentchooser/"));
        qDebug() << "ok, trying in" << services;
        foreach (const QString &service, services) {
            if (!service.endsWith(QLatin1String(".desktop"))) {
                continue;
            }
            KConfig config(service, KConfig::SimpleConfig);
            KConfigGroup cg = config.group(QByteArray());
            const QString type = cg.readEntry("valueName", QString());
            // qDebug() << "    checking" << service << type << application;
            if (matches(type, application)) {
                KConfig store(
                    cg.readPathEntry("storeInFile", QStringLiteral("null")));
                KConfigGroup storeCg(&store,
                                     cg.readEntry("valueSection", QString()));
                const QString exec = storeCg.readPathEntry(
                    cg.readEntry("valueName", "kcm_componenchooser_null"),
                    cg.readEntry("defaultImplementation", QString()));
                if (!exec.isEmpty()) {
                    return exec;
                }

                break;
            }
        }
    }

    return false;
}

QScriptValue ScriptEngine::V1::applicationPath(QScriptContext *context,
                                               QScriptEngine *engine)
{
    Q_UNUSED(engine)
    if (context->argumentCount() == 0) {
        return false;
    }

    const QString application = context->argument(0).toString();
    if (application.isEmpty()) {
        return false;
    }

    // first, check for it in $PATH
    const QString path = QStandardPaths::findExecutable(application);
    if (!path.isEmpty()) {
        return path;
    }

    if (KService::Ptr service = KService::serviceByStorageId(application)) {
        return QStandardPaths::locate(QStandardPaths::ApplicationsLocation,
                                      service->entryPath());
    }

    if (application.contains(QStringLiteral("'"))) {
        // apostrophes just screw up the trader lookups below, so check for it
        return QString();
    }

    // next, consult ksycoca for an app by that name
    KService::List offers = KServiceTypeTrader::self()->query(
        QStringLiteral("Application"),
        QStringLiteral("Name =~ '%1'").arg(application));
    if (offers.isEmpty()) {
        // next, consult ksycoca for an app by that generic name
        offers = KServiceTypeTrader::self()->query(
            QStringLiteral("Application"),
            QStringLiteral("GenericName =~ '%1'").arg(application));
    }

    if (!offers.isEmpty()) {
        KService::Ptr offer = offers.first();
        return QStandardPaths::locate(QStandardPaths::ApplicationsLocation,
                                      offer->entryPath());
    }

    return QString();
}

QScriptValue ScriptEngine::V1::userDataPath(QScriptContext *context,
                                            QScriptEngine *engine)
{
    Q_UNUSED(engine)
    if (context->argumentCount() == 0) {
        return QDir::homePath();
    }

    const QString type = context->argument(0).toString();
    if (type.isEmpty()) {
        return QDir::homePath();
    }

    QStandardPaths::StandardLocation location
        = QStandardPaths::GenericDataLocation;
    if (matches(type, QLatin1String("desktop"))) {
        location = QStandardPaths::DesktopLocation;

    } else if (matches(type, QLatin1String("documents"))) {
        location = QStandardPaths::DocumentsLocation;

    } else if (matches(type, QLatin1String("music"))) {
        location = QStandardPaths::MusicLocation;

    } else if (matches(type, QLatin1String("video"))) {
        location = QStandardPaths::MoviesLocation;

    } else if (matches(type, QLatin1String("downloads"))) {
        location = QStandardPaths::DownloadLocation;

    } else if (matches(type, QLatin1String("pictures"))) {
        location = QStandardPaths::PicturesLocation;

    } else if (matches(type, QLatin1String("config"))) {
        location = QStandardPaths::GenericConfigLocation;
    }

    if (context->argumentCount() > 1) {
        QString loc = QStandardPaths::writableLocation(location);
        loc.append(QDir::separator());
        loc.append(context->argument(1).toString());
        return loc;
    }

    const QStringList &locations = QStandardPaths::standardLocations(location);
    return locations.count() ? locations.first() : QString();
}

QScriptValue ScriptEngine::V1::knownWallpaperPlugins(QScriptContext *context,
                                                     QScriptEngine *engine)
{
    Q_UNUSED(engine)

    QString formFactor;
    if (context->argumentCount() > 0) {
        formFactor = context->argument(0).toString();
    }

    QString constraint;
    if (!formFactor.isEmpty()) {
        constraint.append("[X-Plasma-FormFactors] ~~ '")
            .append(formFactor)
            .append("'");
    }

    const QList<KPluginMetaData> wallpapers
        = KPackage::PackageLoader::self()->listPackages(
            QStringLiteral("Plasma/Wallpaper"), QString());
    QScriptValue rv = engine->newArray(wallpapers.size());
    for (auto wp : wallpapers) {
        rv.setProperty(wp.name(), engine->newArray(0));
    }

    return rv;
}

QScriptValue ScriptEngine::V1::configFile(QScriptContext *context,
                                          QScriptEngine *engine)
{
    ConfigGroup *file = 0;

    if (context->argumentCount() > 0) {
        if (context->argument(0).isString()) {
            file = new ConfigGroup;

            const QString &fileName = context->argument(0).toString();
            const ScriptEngine *env = envFor(engine);
            const Plasma::Corona *corona = env->corona();

            if (fileName == corona->config()->name()) {
                file->setConfig(corona->config());
            } else {
                file->setFile(fileName);
            }

            if (context->argumentCount() > 1) {
                file->setGroup(context->argument(1).toString());
            }

        } else if (ConfigGroup *parent = qobject_cast<ConfigGroup *>(
                     context->argument(0).toQObject())) {
            file = new ConfigGroup(parent);

            if (context->argumentCount() > 1) {
                file->setGroup(context->argument(1).toString());
            }
        }

    } else {
        file = new ConfigGroup;
    }

    QScriptValue v
        = engine->newQObject(file, QScriptEngine::ScriptOwnership,
                             QScriptEngine::ExcludeSuperClassProperties
                                 | QScriptEngine::ExcludeSuperClassMethods);
    return v;
}

QScriptValue ScriptEngine::V1::desktops(QScriptContext *context,
                                        QScriptEngine *engine)
{
    Q_UNUSED(context)

    QScriptValue containments = engine->newArray();
    ScriptEngine *env = envFor(engine);
    int count = 0;

    foreach (Plasma::Containment *c, env->corona()->containments()) {
        // make really sure we get actual desktops, so check for a non empty
        // activty id
        if (!isPanel(c) && !c->activity().isEmpty()) {
            containments.setProperty(count, env->wrap(c));
            ++count;
        }
    }

    containments.setProperty(QStringLiteral("length"), count);
    return containments;
}

QScriptValue ScriptEngine::V1::gridUnit()
{
    return ScriptEngine::gridUnit();
}

QScriptValue ScriptEngine::V1::createContainment(const QString &type, const QString &defaultPlugin,
                                                 QScriptContext *context, QScriptEngine *engine)
{
    const QString plugin = context->argumentCount() > 0
                               ? context->argument(0).toString()
                               : defaultPlugin;

    ScriptEngine *env = envFor(engine);
    auto result = env->createContainment(type, plugin);

    if (!result) {
        return context->throwError(i18n("Could not find a plugin for %1 named %2.", type, plugin));
    }

    return env->wrap(result);
}

} // namespace WorkspaceScripting


