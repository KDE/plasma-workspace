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
#include <QJSValueIterator>
#include <QStandardPaths>
#include <QFutureWatcher>

#include <QDebug>
#include <klocalizedstring.h>
#include <KApplicationTrader>
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
        ScriptArray_forEach_Helper(const QJSValue &array)
            : array(array)
        {
        }

        // operator + is commonly used for these things
        // to avoid having the lambda inside the parenthesis
        template <typename Function>
        void operator+ (Function function) const
        {
            if (!array.isArray()) return;

            int length = array.property("length").toInt();
            for (int i = 0; i < length; ++i) {
                function(array.property(i));
            }
        }

    private:
        const QJSValue &array;
    };

    #define SCRIPT_ARRAY_FOREACH(Variable, Array) \
        ScriptArray_forEach_Helper(Array) + [&] (const QJSValue &Variable)

    class ScriptObject_forEach_Helper {
    public:
        ScriptObject_forEach_Helper(const QJSValue &object)
            : object(object)
        {
        }

        // operator + is commonly used for these things
        // to avoid having the lambda inside the parenthesis
        template <typename Function>
        void operator+ (Function function) const
        {
            QJSValueIterator it(object);
            while (it.hasNext()) {
                it.next();
                function(it.name(), it.value());
            }
        }

    private:
        const QJSValue &object;
    };

    #define SCRIPT_OBJECT_FOREACH(Key, Value, Array) \
        ScriptObject_forEach_Helper(Array) + [&] (const QString &Key, const QJSValue &Value)

    // Case insensitive comparison of two strings
    template <typename StringType>
    inline bool matches(const QString &object, const StringType &string)
    {
        return object.compare(string, Qt::CaseInsensitive) == 0;
    }
}

namespace WorkspaceScripting
{

ScriptEngine::V1::V1(ScriptEngine *parent)
     : QObject(parent),
       m_engine(parent)
{}

ScriptEngine::V1::~V1()
{}

QJSValue ScriptEngine::V1::getApiVersion(const QJSValue &param)
{
    if (param.toInt() != 1) {
        return m_engine->newError(i18n("maximum api version supported is 1"));
    }
    return m_engine->newQObject(this);
}

int ScriptEngine::V1::gridUnit() const
{
    int gridUnit = QFontMetrics(QGuiApplication::font()).boundingRect(QStringLiteral("M")).height();
    if (gridUnit % 2 != 0) {
        gridUnit++;
    }

    return gridUnit;
}

QJSValue ScriptEngine::V1::desktopById(const QJSValue &param) const
{
    //this needs to work also for string of numberls, like "20"
    if (param.isUndefined()) {
        return m_engine->newError(i18n("desktopById required an id"));
    }

    const quint32 id = param.toInt();

    foreach (Plasma::Containment *c, m_engine->m_corona->containments()) {
        if (c->id() == id && !isPanel(c)) {
            return m_engine->wrap(c);
        }
    }

    return QJSValue();
}

QJSValue ScriptEngine::V1::desktopsForActivity(const QJSValue &actId) const
{
    if (!actId.isString()) {
        return m_engine->newError(i18n("desktopsForActivity requires an id"));
    }

    QJSValue containments = m_engine->newArray();
    int count = 0;

    const QString id = actId.toString();

    const auto result = m_engine->desktopsForActivity(id);

    for (Containment* c: result) {
        containments.setProperty(count, m_engine->newQObject(c));
        ++count;
    }

    containments.setProperty(QStringLiteral("length"), count);
    return containments;
}

QJSValue ScriptEngine::V1::desktopForScreen(const QJSValue &param) const
{
    //this needs to work also for string of numberls, like "20"
    if (param.isUndefined()) {
        return m_engine->newError(i18n("activityForScreen requires a screen id"));
    }

    const uint screen = param.toInt();
    return m_engine->wrap(m_engine->m_corona->containmentForScreen(screen));
}

QJSValue ScriptEngine::V1::createActivity(const QJSValue &nameParam, const QString &pluginParam)
{
    if (!nameParam.isString()) {
        return m_engine->newError(i18n("createActivity required the activity name"));
    }

    QString plugin = pluginParam;
    const QString name = nameParam.toString();

    KActivities::Controller controller;

    // This is not the nicest way to do this, but createActivity
    // is a synchronous API :/
    QFuture<QString> futureId = controller.addActivity(name);
    awaitFuture(futureId);

    QString id = futureId.result();

    qDebug() << "Setting default Containment plugin:" << plugin;

    ShellCorona *sc = qobject_cast<ShellCorona *>(m_engine->m_corona);
    StandaloneAppCorona *ac = qobject_cast<StandaloneAppCorona *>(m_engine->m_corona);
    if (sc) {
        if (plugin.isEmpty() || plugin == QLatin1String("undefined")) {
            plugin = sc->defaultContainmentPlugin();
        }
        sc->insertActivity(id, plugin);
    } else if (ac) {
        if (plugin.isEmpty() || plugin == QLatin1String("undefined")) {
            KConfigGroup shellCfg = KConfigGroup(KSharedConfig::openConfig(m_engine->m_corona->package().filePath("defaults")), "Desktop");
            plugin = shellCfg.readEntry("Containment", "org.kde.desktopcontainment");
        }
        ac->insertActivity(id, plugin);
    }

    return m_engine->toScriptValue<QString>(id);
}

QJSValue ScriptEngine::V1::setCurrentActivity(const QJSValue &param)
{
    if (!param.isString()) {
        return m_engine->newError(i18n("setCurrentActivity required the activity id"));
    }

    const QString id = param.toString();

    KActivities::Controller controller;

    QFuture<bool> task = controller.setCurrentActivity(id);
    awaitFuture(task);

    return task.result();
}

QJSValue ScriptEngine::V1::setActivityName(const QJSValue &idParam, const QJSValue &nameParam)
{
    if (!idParam.isString() || !nameParam.isString()) {
        return m_engine->newError(i18n("setActivityName required the activity id and name"));
    }


    const QString id = idParam.toString();
    const QString name = nameParam.toString();

    KActivities::Controller controller;

    QFuture<void> task = controller.setActivityName(id, name);
    awaitFuture(task);
    return QJSValue();
}

QJSValue ScriptEngine::V1::activityName(const QJSValue &idParam) const
{
    if (!idParam.isString()) {
        return m_engine->newError(i18n("activityName required the activity id"));
    }

    const QString id = idParam.toString();

    KActivities::Info info(id);

    return QJSValue(info.name());
}

QString ScriptEngine::V1::currentActivity() const
{
    KActivities::Consumer consumer;
    return consumer.currentActivity();
}

QJSValue ScriptEngine::V1::activities() const
{
    QJSValue acts = m_engine->newArray();
    int count = 0;

    const auto result = m_engine->availableActivities();

    for (const auto a : result) {
        acts.setProperty(count, a);
        ++count;
    }
    acts.setProperty(QStringLiteral("length"), count);

    return acts;
}

// Utility function to process configs and config groups
template <typename Object>
void loadSerializedConfigs(Object *object, const QJSValue &configs)
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
            object->writeConfig(key, value);
        };
    };
}

QJSValue ScriptEngine::V1::loadSerializedLayout(const QJSValue &data)
{
    if (!data.isObject()) {
        return m_engine->newError(i18n("loadSerializedLayout requires the JSON object to deserialize from"));
    }

    if (data.property("serializationFormatVersion").toInt() != 1) {
        return m_engine->newError(i18n("loadSerializedLayout: invalid version of the serialized object"));
    }

    const auto desktops = m_engine->desktopsForActivity(KActivities::Consumer().currentActivity());
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

            auto appletObject = desktop->addWidget( appletData.property("plugin"),
                appletData.property("geometry.x").toInt()      * gridUnit(),
                appletData.property("geometry.y").toInt()      * gridUnit(),
                appletData.property("geometry.width").toInt()  * gridUnit(),
                appletData.property("geometry.height").toInt() * gridUnit());

            if (auto applet = qobject_cast<Widget*>(appletObject.toQObject())) {
                // Now, lets go through the configs for the applet
                loadSerializedConfigs(applet, appletData.property("config"));
            }
        };

        count++;
    };

    // qDebug() << "PANEL DESERIALIZATION: Loading panels...";

    SCRIPT_ARRAY_FOREACH(panelData, data.property("panels")) {
        const auto panel = qobject_cast<Panel *>(m_engine->createContainmentWrapper(
            QStringLiteral("Panel"), QStringLiteral("org.kde.panel")));

        Q_ASSERT(panel);

        // Basic panel setup
        panel->setLocation(panelData.property("location").toString());
        panel->setHeight(panelData.property("height").toNumber() * gridUnit());
        panel->setMaximumLength(panelData.property("maximumLength").toNumber() * gridUnit());
        panel->setMinimumLength(panelData.property("minimumLength").toNumber() * gridUnit());
        panel->setOffset(panelData.property("offset").toNumber() * gridUnit());
        panel->setAlignment(panelData.property("alignment").toString());
        panel->setHiding(panelData.property("hiding").toString());

        // Loading the config for the panel
        loadSerializedConfigs(panel, panelData.property("config"));

        // Now dealing with the applets
        SCRIPT_ARRAY_FOREACH(appletData, panelData.property("applets")) {
            // qDebug() << "PANEL DESERIALIZATION: Applet: " << appletData.toString();

            auto appletObject = panel->addWidget(appletData.property("plugin"));
            // qDebug() << "PANEL DESERIALIZATION: addWidget"
            //      << appletData.property("plugin").toString()
            //      ;

            if (auto applet = qobject_cast<Widget*>(appletObject.toQObject())) {
                // Now, lets go through the configs for the applet
                loadSerializedConfigs(applet, appletData.property("config"));
            }
        };
    };

    return QJSValue();
}

QJSValue ScriptEngine::V1::newPanel(const QString &plugin)
{
    return createContainment(QStringLiteral("Panel"), QStringLiteral("org.kde.panel"), plugin);
}

QJSValue ScriptEngine::V1::panelById(const QJSValue &idParam) const
{
    //this needs to work also for string of numberls, like "20"
    if (idParam.isUndefined()) {
        return m_engine->newError(i18n("panelById requires an id"));
    }

    const quint32 id = idParam.toInt();

    foreach (Plasma::Containment *c, m_engine->m_corona->containments()) {
        if (c->id() == id && isPanel(c)) {
            return m_engine->wrap(c);
        }
    }

    return QJSValue();
}

QJSValue ScriptEngine::V1::desktops() const
{
    QJSValue containments = m_engine->newArray();
    int count = 0;

    const auto result = m_engine->m_corona->containments();

    for (const auto c : result) {
        // make really sure we get actual desktops, so check for a non empty
        // activity id
        if (!isPanel(c) && !c->activity().isEmpty()) {
            containments.setProperty(count, m_engine->wrap(c));
            ++count;
        }
    }

    containments.setProperty(QStringLiteral("length"), count);
    return containments;
}

QJSValue ScriptEngine::V1::panels() const
{
    QJSValue panels = m_engine->newArray();
    int count = 0;

    const auto result = m_engine->m_corona->containments();

    for (const auto c : result) {
        panels.setProperty(count, m_engine->wrap(c));
        ++count;
    }
    panels.setProperty(QStringLiteral("length"), count);

    return panels;
}

bool ScriptEngine::V1::fileExists(const QString &path) const
{
    if (path.isEmpty()) {
        return false;
    }

    QFile f(KShell::tildeExpand(path));
    return f.exists();
}

bool ScriptEngine::V1::loadTemplate(const QString &layout)
{
    if (layout.isEmpty() || layout.contains(QLatin1Char('\''))) {
        // qDebug() << "layout is empty";
        return false;
    }

    auto filter = [&layout](const KPluginMetaData &md) -> bool
    {
        return md.pluginId() == layout && KPluginMetaData::readStringList(md.rawData(), QStringLiteral("X-Plasma-ContainmentCategories")).contains(QLatin1String("panel"));
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
        ShellCorona *sc = qobject_cast<ShellCorona *>(m_engine->m_corona);
        if (sc) {
            const QString overridePackagePath = sc->lookAndFeelPackage().path() + QLatin1String("contents/layouts/") + pluginData.pluginId();

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

    ScriptEngine *engine = new ScriptEngine(m_engine->corona(), this);
    engine->globalObject().setProperty(QStringLiteral("templateName"), pluginData.name());
    engine->globalObject().setProperty(QStringLiteral("templateComment"), pluginData.description());

    engine->evaluateScript(script, path);

    engine->deleteLater();
    return true;
}

bool ScriptEngine::V1::applicationExists(const QString &application) const
{
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

    if (application.contains(QLatin1Char('\''))) {
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

QJSValue ScriptEngine::V1::defaultApplication(const QString &application, bool storageId) const
{
    if (application.isEmpty()) {
        return false;
    }

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
                = KApplicationTrader::preferredService(QStringLiteral("text/html"));
            if (htmlApp) {
                browserApp = storageId ? htmlApp->storageId() : htmlApp->exec();
            }
        } else if (browserApp.startsWith('!')) {
            browserApp.remove(0, 1);
        }

        return onlyExec(browserApp);

    } else if (matches(application, QLatin1String("terminal"))) {
        KConfigGroup confGroup(KSharedConfig::openConfig(), "General");
        return onlyExec(confGroup.readPathEntry("TerminalApplication",
                                                QStringLiteral("konsole")));

    } else if (matches(application, QLatin1String("filemanager"))) {
        KService::Ptr service = KApplicationTrader::preferredService(
            QStringLiteral("inode/directory"));
        if (service) {
            return storageId ? service->storageId() : onlyExec(service->exec());
        }

    } else if (matches(application, QLatin1String("windowmanager"))) {
        KConfig cfg(QStringLiteral("ksmserverrc"), KConfig::NoGlobals);
        KConfigGroup confGroup(&cfg, "General");
        return onlyExec(
            confGroup.readEntry("windowManager", QStringLiteral("kwin")));

    } else if (KService::Ptr service = KApplicationTrader::preferredService(application)) {
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

QJSValue ScriptEngine::V1::applicationPath(const QString &application) const
{
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

    if (application.contains(QLatin1Char('\''))) {
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

QJSValue ScriptEngine::V1::userDataPath(const QString &type, const QString &path) const
{
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

    if (!path.isEmpty()) {
        QString loc = QStandardPaths::writableLocation(location);
        loc.append(QDir::separator());
        loc.append(path);
        return loc;
    }

    const QStringList &locations = QStandardPaths::standardLocations(location);
    return locations.count() ? locations.first() : QString();
}

QJSValue ScriptEngine::V1::knownWallpaperPlugins(const QString &formFactor) const
{
    QString constraint;
    if (!formFactor.isEmpty()) {
        constraint.append("[X-Plasma-FormFactors] ~~ '")
            .append(formFactor)
            .append("'");
    }

    const QList<KPluginMetaData> wallpapers
        = KPackage::PackageLoader::self()->listPackages(
            QStringLiteral("Plasma/Wallpaper"), QString());
    QJSValue rv = m_engine->newArray(wallpapers.size());
    for (auto wp : wallpapers) {
        rv.setProperty(wp.name(), m_engine->newArray(0));
    }

    return rv;
}

QJSValue ScriptEngine::V1::configFile(const QJSValue &config, const QString &group)
{
    ConfigGroup *file = nullptr;

    if (!config.isUndefined()) {
        if (config.isString()) {
            file = new ConfigGroup;

            const Plasma::Corona *corona = m_engine->corona();
            const QString &fileName = config.toString();

            if (fileName == corona->config()->name()) {
                file->setConfig(corona->config());
            } else {
                file->setFile(fileName);
            }

            if (!group.isEmpty()) {
                file->setGroup(group);
            }

        } else if (ConfigGroup *parent = qobject_cast<ConfigGroup *>(
                     config.toQObject())) {
            file = new ConfigGroup(parent);

            if (!group.isEmpty()) {
                file->setGroup(group);
            }
        }

    } else {
        file = new ConfigGroup;
    }

    QJSValue v = m_engine->newQObject(file);
    return v;
}

void ScriptEngine::V1::setImmutability(const QString &immutability)
{
    if (immutability.isEmpty()) {
        return;
    }

    if (immutability == QLatin1String("systemImmutable")) {
        m_engine->corona()->setImmutability(Plasma::Types::SystemImmutable);
    } else if (immutability == QLatin1String("userImmutable")) {
        m_engine->corona()->setImmutability(Plasma::Types::UserImmutable);
    } else {
        m_engine->corona()->setImmutability(Plasma::Types::Mutable);
    }

    return;
}

QString ScriptEngine::V1::immutability() const
{
    switch (m_engine->corona()->immutability()) {
    case Plasma::Types::SystemImmutable:
        return QLatin1String("systemImmutable");
    case Plasma::Types::UserImmutable:
        return QLatin1String("userImmutable");
    default:
        return QLatin1String("mutable");
    }
}

QJSValue ScriptEngine::V1::createContainment(const QString &type, const QString &defaultPlugin, const QString &plugin)
{
    const QString actualPlugin = plugin.isEmpty()
                               ? defaultPlugin
                               : plugin;

    auto result = m_engine->createContainmentWrapper(type, actualPlugin);

    if (!result) {
        return m_engine->newError(i18n("Could not find a plugin for %1 named %2.", type, actualPlugin));
    }

    return m_engine->newQObject(result);
}

} // namespace WorkspaceScripting


