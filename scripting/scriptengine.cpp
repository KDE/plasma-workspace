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

#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScriptValueIterator>

#include <KDebug>
#include <kdeversion.h>
#include <KGlobalSettings>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>
#include <KShell>
#include <KStandardDirs>

// KIO
#include <kemailsettings.h> // no camelcase include

#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/Package>
#include <Plasma/Wallpaper>

#include "appinterface.h"
#include "containment.h"
#include "i18n.h"
#include "layouttemplatepackagestructure.h"
#include "widget.h"

QScriptValue constructQRectFClass(QScriptEngine *engine);

namespace WorkspaceScripting
{

ScriptEngine::ScriptEngine(Plasma::Corona *corona, QObject *parent)
    : QScriptEngine(parent),
      m_corona(corona)
{
    Q_ASSERT(m_corona);
    AppInterface *interface = new AppInterface(this);
    connect(interface, SIGNAL(print(QString)), this, SIGNAL(print(QString)));
    m_scriptSelf = newQObject(interface, QScriptEngine::QtOwnership,
                              QScriptEngine::ExcludeSuperClassProperties |
                              QScriptEngine::ExcludeSuperClassMethods);
    setupEngine();
    connect(this, SIGNAL(signalHandlerException(QScriptValue)), this, SLOT(exception(QScriptValue)));
    bindI18N(this);
}

ScriptEngine::~ScriptEngine()
{
}

QScriptValue ScriptEngine::activityById(QScriptContext *context, QScriptEngine *engine)
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

QScriptValue ScriptEngine::activityForScreen(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("activityForScreen requires a screen id"));
    }

    const uint screen = context->argument(0).toInt32();
    const uint desktop = context->argumentCount() > 1 ? context->argument(1).toInt32() : -1;
    ScriptEngine *env = envFor(engine);
    return env->wrap(env->m_corona->containmentForScreen(screen, desktop));
}

QScriptValue ScriptEngine::newActivity(QScriptContext *context, QScriptEngine *engine)
{
    return createContainment("desktop", "desktop", context, engine);
}

QScriptValue ScriptEngine::newPanel(QScriptContext *context, QScriptEngine *engine)
{
    return createContainment("panel", "panel", context, engine);
}

QScriptValue ScriptEngine::createContainment(const QString &type, const QString &defaultPlugin,
                                             QScriptContext *context, QScriptEngine *engine)
{
    QString plugin = context->argumentCount() > 0 ? context->argument(0).toString() :
                                                    defaultPlugin;

    bool exists = false;
    const KPluginInfo::List list = Plasma::Containment::listContainmentsOfType(type);
    foreach (const KPluginInfo &info, list) {
        if (info.pluginName() == plugin) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        return context->throwError(i18n("Could not find a plugin for %1 named %2.", type, plugin));
    }


    ScriptEngine *env = envFor(engine);
    Plasma::Containment *c = env->m_corona->addContainment(plugin);
    if (c) {
        if (type == "panel") {
            // some defaults
            c->setScreen(env->defaultPanelScreen());
            c->setLocation(Plasma::TopEdge);
        }
        c->updateConstraints(Plasma::AllConstraints | Plasma::StartupCompletedConstraint);
        c->flushPendingConstraintsEvents();
        emit env->createPendingPanelViews();
    }

    return env->wrap(c);
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
    Containment *wrapper = new Containment(c);
    return wrap(wrapper);
}

QScriptValue ScriptEngine::wrap(Containment *c)
{
    QScriptValue v = newQObject(c, QScriptEngine::ScriptOwnership,
                                QScriptEngine::ExcludeSuperClassProperties |
                                QScriptEngine::ExcludeSuperClassMethods);
    v.setProperty("widgetById", newFunction(Containment::widgetById));
    v.setProperty("addWidget", newFunction(Containment::addWidget));
    v.setProperty("widgets", newFunction(Containment::widgets));

    return v;
}

int ScriptEngine::defaultPanelScreen() const
{
    return qApp ? qApp->desktop()->primaryScreen() : 0;
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

QScriptValue ScriptEngine::panelById(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("activityById requires an id"));
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

QScriptValue ScriptEngine::panels(QScriptContext *context, QScriptEngine *engine)
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

    panels.setProperty("length", count);
    return panels;
}

QScriptValue ScriptEngine::fileExists(QScriptContext *context, QScriptEngine *engine)
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

QScriptValue ScriptEngine::loadTemplate(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)
    if (context->argumentCount() == 0) {
        kDebug() << "no arguments";
        return false;
    }

    const QString layout = context->argument(0).toString();
    if (layout.isEmpty() || layout.contains("'")) {
        kDebug() << "layout is empty";
        return false;
    }

    const QString constraint = QString("[X-Plasma-Shell] == '%1' and [X-KDE-PluginInfo-Name] == '%2'")
                                      .arg(KGlobal::mainComponent().componentName(),layout);
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/LayoutTemplate", constraint);

    if (offers.isEmpty()) {
        kDebug() << "offers fail" << constraint;
        return false;
    }

    Plasma::PackageStructure::Ptr structure(new LayoutTemplatePackageStructure);
    KPluginInfo info(offers.first());
    const QString path = KStandardDirs::locate("data", structure->defaultPackageRoot() + '/' + info.pluginName() + '/');
    if (path.isEmpty()) {
        kDebug() << "script path is empty";
        return false;
    }

    Plasma::Package package(path, structure);
    const QString scriptFile = package.filePath("mainscript");
    if (scriptFile.isEmpty()) {
        kDebug() << "scriptfile is empty";
        return false;
    }

    QFile file(scriptFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        kWarning() << i18n("Unable to load script file: %1", path);
        return false;
    }

    QString script = file.readAll();
    if (script.isEmpty()) {
        kDebug() << "script is empty";
        return false;
    }

    ScriptEngine *env = envFor(engine);
    env->globalObject().setProperty("templateName", env->newVariant(info.name()), QScriptValue::ReadOnly | QScriptValue::Undeletable);
    env->globalObject().setProperty("templateComment", env->newVariant(info.comment()), QScriptValue::ReadOnly | QScriptValue::Undeletable);

    QScriptValue rv = env->newObject();
    QScriptContext *ctx = env->pushContext();
    ctx->setThisObject(rv);

    env->evaluateScript(script, path);

    env->popContext();
    return rv;
}

QScriptValue ScriptEngine::applicationExists(QScriptContext *context, QScriptEngine *engine)
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
    if (!KStandardDirs::findExe(application).isEmpty()) {
        return true;
    }

    if (KService::serviceByStorageId(application)) {
        return true;
    }

    if (application.contains("'")) {
        // apostrophes just screw up the trader lookups below, so check for it
        return false;
    }

    // next, consult ksycoca for an app by that name
    if (!KServiceTypeTrader::self()->query("Application", QString("Name =~ '%1'").arg(application)).isEmpty()) {
        return true;
    }

    // next, consult ksycoca for an app by that generic name
    if (!KServiceTypeTrader::self()->query("Application", QString("GenericName =~ '%1'").arg(application)).isEmpty()) {
        return true;
    }

    return false;
}

QString ScriptEngine::onlyExec(const QString &commandLine)
{
    if (commandLine.isEmpty()) {
        return commandLine;
    }

    return KShell::splitArgs(commandLine, KShell::TildeExpand).first();
}

QScriptValue ScriptEngine::defaultApplication(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)
    if (context->argumentCount() == 0) {
        return false;
    }

    const QString application = context->argument(0).toString();
    if (application.isEmpty()) {
        return false;
    }

    const bool storageId = context->argumentCount() < 2 ? false : context->argument(1).toBool();

    // FIXME: there are some pretty horrible hacks below, in the sense that they assume a very
    // specific implementation system. there is much room for improvement here. see
    // kdebase-runtime/kcontrol/componentchooser/ for all the gory details ;)
    if (application.compare("mailer", Qt::CaseInsensitive) == 0) {
        KEMailSettings settings;

        // in KToolInvocation, the default is kmail; but let's be friendlier :)
        QString command = settings.getSetting(KEMailSettings::ClientProgram);
        if (command.isEmpty()) {
            if (KService::Ptr kontact = KService::serviceByStorageId("kontact")) {
                return storageId ? kontact->storageId() : onlyExec(kontact->exec());
            } else if (KService::Ptr kmail = KService::serviceByStorageId("kmail")) {
                return storageId ? kmail->storageId() : onlyExec(kmail->exec());
            }
        }

        if (!command.isEmpty()) {
            if (settings.getSetting(KEMailSettings::ClientTerminal) == "true") {
                KConfigGroup confGroup(KGlobal::config(), "General");
                const QString preferredTerminal = confGroup.readPathEntry("TerminalApplication", QString::fromLatin1("konsole"));
                command = preferredTerminal + QString::fromLatin1(" -e ") + command;
            }

            return command;
        }
    } else if (application.compare("browser", Qt::CaseInsensitive) == 0) {
        KConfigGroup config(KGlobal::config(), "General");
        QString browserApp = config.readPathEntry("BrowserApplication", QString());
        if (browserApp.isEmpty()) {
            const KService::Ptr htmlApp = KMimeTypeTrader::self()->preferredService(QLatin1String("text/html"));
            if (htmlApp) {
                browserApp = storageId ? htmlApp->storageId() : htmlApp->exec();
            }
        } else if (browserApp.startsWith('!')) {
            browserApp = browserApp.mid(1);
        }

        return onlyExec(browserApp);
    } else if (application.compare("terminal", Qt::CaseInsensitive) == 0) {
        KConfigGroup confGroup(KGlobal::config(), "General");
        return onlyExec(confGroup.readPathEntry("TerminalApplication", QString::fromLatin1("konsole")));
    } else if (application.compare("filemanager", Qt::CaseInsensitive) == 0) {
        KService::Ptr service = KMimeTypeTrader::self()->preferredService("inode/directory");
        if (service) {
            return storageId ? service->storageId() : onlyExec(service->exec());
        }
    } else if (application.compare("windowmanager", Qt::CaseInsensitive) == 0) {
        KConfig cfg("ksmserverrc", KConfig::NoGlobals);
        KConfigGroup confGroup(&cfg, "General");
        return onlyExec(confGroup.readEntry("windowManager", QString::fromLatin1("kwin")));
    } else if (KService::Ptr service = KMimeTypeTrader::self()->preferredService(application)) {
        return storageId ? service->storageId() : onlyExec(service->exec());
    } else {
        // try the files in share/apps/kcm_componentchooser/
        const QStringList services = KGlobal::dirs()->findAllResources("data","kcm_componentchooser/*.desktop", KStandardDirs::NoDuplicates);
        //kDebug() << "ok, trying in" << services.count();
        foreach (const QString &service, services) {
            KConfig config(service, KConfig::SimpleConfig);
            KConfigGroup cg = config.group(QByteArray());
            const QString type = cg.readEntry("valueName", QString());
            //kDebug() << "    checking" << service << type << application;
            if (type.compare(application, Qt::CaseInsensitive) == 0) {
                KConfig store(cg.readPathEntry("storeInFile", "null"));
                KConfigGroup storeCg(&store, cg.readEntry("valueSection", QString()));
                const QString exec = storeCg.readPathEntry(cg.readEntry("valueName", "kcm_componenchooser_null"),
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

QScriptValue ScriptEngine::applicationPath(QScriptContext *context, QScriptEngine *engine)
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
    const QString path = KStandardDirs::findExe(application);
    if (!path.isEmpty()) {
        return path;
    }

    if (KService::Ptr service = KService::serviceByStorageId(application)) {
        return KStandardDirs::locate("apps", service->entryPath());
    }

    if (application.contains("'")) {
        // apostrophes just screw up the trader lookups below, so check for it
        return QString();
    }

    // next, consult ksycoca for an app by that name
    KService::List offers = KServiceTypeTrader::self()->query("Application", QString("Name =~ '%1'").arg(application));
    if (offers.isEmpty()) {
        // next, consult ksycoca for an app by that generic name
        offers = KServiceTypeTrader::self()->query("Application", QString("GenericName =~ '%1'").arg(application));
    }

    if (!offers.isEmpty()) {
        KService::Ptr offer = offers.first();
        return KStandardDirs::locate("apps", offer->entryPath());
    }

    return QString();
}

QScriptValue ScriptEngine::userDataPath(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)
    if (context->argumentCount() == 0) {
        return QDir::homePath();
    }

    const QString type = context->argument(0).toString();
    if (type.isEmpty()) {
        return QDir::homePath();
    }

    if (context->argumentCount() > 1) {
        const QString filename = context->argument(1).toString();
        return KStandardDirs::locateLocal(type.toLatin1(), filename);
    }

    if (type.compare("desktop", Qt::CaseInsensitive) == 0) {
        return KGlobalSettings::desktopPath();
    } else if (type.compare("autostart", Qt::CaseInsensitive) == 0) {
        return KGlobalSettings::autostartPath();
    } else if (type.compare("documents", Qt::CaseInsensitive) == 0) {
        return KGlobalSettings::documentPath();
    } else if (type.compare("music", Qt::CaseInsensitive) == 0) {
        return KGlobalSettings::musicPath();
    } else if (type.compare("video", Qt::CaseInsensitive) == 0) {
        return KGlobalSettings::videosPath();
    } else if (type.compare("downloads", Qt::CaseInsensitive) == 0) {
        return KGlobalSettings::downloadPath();
    } else if (type.compare("pictures", Qt::CaseInsensitive) == 0) {
        return KGlobalSettings::picturesPath();
    }

    return QString();
}

QScriptValue ScriptEngine::knownWallpaperPlugins(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    QString formFactor;
    if (context->argumentCount() > 0) {
        formFactor = context->argument(0).toString();
    }

    QString constraint;
    if (!formFactor.isEmpty()) {
        constraint.append("[X-Plasma-FormFactors] ~~ '").append(formFactor).append("'");
    }

    KService::List services = KServiceTypeTrader::self()->query("Plasma/Wallpaper", constraint);
    QScriptValue rv = engine->newArray(services.size());
    foreach (const KService::Ptr service, services) {
        QList<KServiceAction> modeActions = service->actions();
        QScriptValue modes = engine->newArray(modeActions.size());
        int i = 0;
        foreach (const KServiceAction &action, modeActions) {
            modes.setProperty(i++, action.name());
        }

        rv.setProperty(service->name(), modes);
    }

    return rv;
}

void ScriptEngine::setupEngine()
{
    QScriptValue v = globalObject();
    QScriptValueIterator it(v);
    while (it.hasNext()) {
        it.next();
        // we provide our own print implementation, but we want the rest
        if (it.name() != "print") {
            m_scriptSelf.setProperty(it.name(), it.value());
        }
    }

    m_scriptSelf.setProperty("QRectF", constructQRectFClass(this));
    m_scriptSelf.setProperty("Activity", newFunction(ScriptEngine::newActivity));
    m_scriptSelf.setProperty("Panel", newFunction(ScriptEngine::newPanel));
    m_scriptSelf.setProperty("activities", newFunction(ScriptEngine::activities));
    m_scriptSelf.setProperty("activityById", newFunction(ScriptEngine::activityById));
    m_scriptSelf.setProperty("activityForScreen", newFunction(ScriptEngine::activityForScreen));
    m_scriptSelf.setProperty("panelById", newFunction(ScriptEngine::panelById));
    m_scriptSelf.setProperty("panels", newFunction(ScriptEngine::panels));
    m_scriptSelf.setProperty("fileExists", newFunction(ScriptEngine::fileExists));
    m_scriptSelf.setProperty("loadTemplate", newFunction(ScriptEngine::loadTemplate));
    m_scriptSelf.setProperty("applicationExists", newFunction(ScriptEngine::applicationExists));
    m_scriptSelf.setProperty("defaultApplication", newFunction(ScriptEngine::defaultApplication));
    m_scriptSelf.setProperty("userDataPath", newFunction(ScriptEngine::userDataPath));
    m_scriptSelf.setProperty("applicationPath", newFunction(ScriptEngine::applicationPath));
    m_scriptSelf.setProperty("knownWallpaperPlugins", newFunction(ScriptEngine::knownWallpaperPlugins));

    setGlobalObject(m_scriptSelf);
}

bool ScriptEngine::isPanel(const Plasma::Containment *c)
{
    if (!c) {
        return false;
    }

    return c->containmentType() == Plasma::Containment::PanelContainment ||
           c->containmentType() == Plasma::Containment::CustomPanelContainment;
}

QScriptValue ScriptEngine::activities(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(context)

    QScriptValue containments = engine->newArray();
    ScriptEngine *env = envFor(engine);
    int count = 0;

    foreach (Plasma::Containment *c, env->corona()->containments()) {
        if (!isPanel(c)) {
            containments.setProperty(count, env->wrap(c));
            ++count;
        }
    }

    containments.setProperty("length", count);
    return containments;
}

Plasma::Corona *ScriptEngine::corona() const
{
    return m_corona;
}

bool ScriptEngine::evaluateScript(const QString &script, const QString &path)
{
    //kDebug() << "evaluating" << m_editor->toPlainText();
    evaluate(script, path);
    if (hasUncaughtException()) {
        //kDebug() << "catch the exception!";
        QString error = i18n("Error: %1 at line %2\n\nBacktrace:\n%3",
                             uncaughtException().toString(),
                             QString::number(uncaughtExceptionLineNumber()),
                             uncaughtExceptionBacktrace().join("\n  "));
        emit printError(error);
        return false;
    }

    return true;
}

void ScriptEngine::exception(const QScriptValue &value)
{
    //kDebug() << "exception caught!" << value.toVariant();
    emit printError(value.toVariant().toString());
}

QStringList ScriptEngine::pendingUpdateScripts()
{
    const QString appName = KGlobal::activeComponent().aboutData()->appName();
    QStringList scripts = KGlobal::dirs()->findAllResources("data", appName + "/updates/*.js");
    QStringList scriptPaths;

    if (scripts.isEmpty()) {
        //kDebug() << "no update scripts";
        return scriptPaths;
    }

    KConfigGroup cg(KGlobal::config(), "Updates");
    QStringList performed = cg.readEntry("performed", QStringList());
    const QString localDir = KGlobal::dirs()->localkdedir();
    const QString localXdgDir = KGlobal::dirs()->localxdgdatadir();

    foreach (const QString &script, scripts) {
        if (performed.contains(script)) {
            continue;
        }

        if (script.startsWith(localDir) || script.startsWith(localXdgDir)) {
            kDebug() << "skipping user local script: " << script;
            continue;
        }

        scriptPaths.append(script);
        performed.append(script);
    }

    cg.writeEntry("performed", performed);
    KGlobal::config()->sync();
    return scriptPaths;
}

QStringList ScriptEngine::defaultLayoutScripts()
{
    const QString appName = KGlobal::activeComponent().aboutData()->appName();
    QStringList appNameDirs = KGlobal::dirs()->findDirs("data", appName);
    QStringList scripts;
    QDir appDir;
    QFileInfoList scriptList;

    foreach (const QString &appNameDir, appNameDirs) {
        appDir.setPath(appNameDir + QLatin1String("init/"));
        if (appDir.exists()) {
            scriptList = appDir.entryInfoList(QStringList("*.js"),
                                            QDir::NoFilter,
                                            QDir::Name);
            foreach (const QFileInfo &script, scriptList) {
                if (script.exists()) {
                    scripts.append(script.absoluteFilePath());
                }
            }
        }
    }

    QStringList scriptPaths;

    if (scripts.isEmpty()) {
        //kDebug() << "no javascript based layouts";
        return scriptPaths;
    }

    const QString localDir = KGlobal::dirs()->localkdedir();
    const QString localXdgDir = KGlobal::dirs()->localxdgdatadir();

    QSet<QString> scriptNames;
    foreach (const QString &script, scripts) {
        if (script.startsWith(localDir) || script.startsWith(localXdgDir)) {
            kDebug() << "skipping user local script: " << script;
            continue;
        }

        QFileInfo f(script);
        QString filename = f.fileName();
        if (!scriptNames.contains(filename)) {
            scriptNames.insert(filename);
            scriptPaths.append(script);
        }
    }

    return scriptPaths;
}

}

#include "scriptengine.moc"

