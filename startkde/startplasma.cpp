/*
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2014-2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>
    SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <config-startplasma.h>

#include <canberra.h>

#include <ranges>

#include <QDir>
#include <QEventLoop>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

#include <KConfig>
#include <KConfigGroup>
#include <KNotifyConfig>
#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <KSharedConfig>

#include <unistd.h>

#include <autostartscriptdesktopfile.h>

#include <KUpdateLaunchEnvironmentJob>

#include "startplasma.h"

#include "../config-workspace.h"
#include "../kcms/lookandfeel/lookandfeelmanager.h"
#include "debug.h"

using namespace Qt::StringLiterals;

QTextStream out(stderr);

void sigtermHandler(int signalNumber)
{
    Q_UNUSED(signalNumber)
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->exit(-1);
    }
}

void messageBox(const QString &text)
{
    out << text;
    runSync(QStringLiteral("xmessage"), {QStringLiteral("-geometry"), QStringLiteral("500x100"), text});
}

QStringList allServices(const QLatin1String &prefix)
{
    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    QStringList names;

    std::copy_if(services.cbegin(), services.cend(), std::back_inserter(names), [&prefix](const QString &serviceName) {
        return serviceName.startsWith(prefix);
    });

    return names;
}

void gentleTermination(QProcess *p)
{
    if (p->state() != QProcess::Running) {
        return;
    }

    p->terminate();

    // Wait longer for a session than a greeter
    if (!p->waitForFinished(5000)) {
        p->kill();
        if (!p->waitForFinished(5000)) {
            qCWarning(PLASMA_STARTUP) << "Could not fully finish the process" << p->program();
        }
    }
}

int runSync(const QString &program, const QStringList &args, const QStringList &env)
{
    QProcess p;
    if (!env.isEmpty())
        p.setEnvironment(QProcess::systemEnvironment() << env);
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(program, args);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, &p, [&p] {
        gentleTermination(&p);
    });
    //     qCDebug(PLASMA_STARTUP) << "started..." << program << args;
    p.waitForFinished(-1);
    if (p.exitCode()) {
        qCWarning(PLASMA_STARTUP) << program << args << "exited with code" << p.exitCode();
    }
    return p.exitCode();
}

template<typename T>
concept ViewType = std::same_as<QByteArrayView, T> || std::same_as<QStringView, T>;
inline bool isShellVariable(ViewType auto name)
{
    return name == "_"_L1 || name == "SHELL"_L1 || name.startsWith("SHLVL"_L1);
}

inline bool isSessionVariable(QStringView name)
{
    // Check is variable is specific to session.
    return name == "DISPLAY"_L1 || name == "XAUTHORITY"_L1 || //
        name == "WAYLAND_DISPLAY"_L1 || name == "WAYLAND_SOCKET"_L1 || //
        name.startsWith("XDG_"_L1);
}

void setEnvironmentVariable(const char *name, QByteArrayView value)
{
    if (qgetenv(name) != value) {
        qputenv(name, value);
    }
}

void sourceFiles(const QStringList &files)
{
    QStringList filteredFiles;
    std::copy_if(files.begin(), files.end(), std::back_inserter(filteredFiles), [](const QString &i) {
        return QFileInfo(i).isReadable();
    });

    if (filteredFiles.isEmpty())
        return;

    filteredFiles.prepend(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR "/plasma-sourceenv.sh"));

    QProcess p;
    p.start(QStringLiteral("/bin/sh"), filteredFiles);
    p.waitForFinished(-1);

    const QByteArrayList fullEnv = p.readAllStandardOutput().split('\0');
    for (const QByteArray &env : fullEnv) {
        const int idx = env.indexOf('=');
        if (idx <= 0) [[unlikely]] {
            continue;
        }

        const auto name = env.sliced(0, idx);
        if (isShellVariable(QByteArrayView(name))) {
            continue;
        }
        setEnvironmentVariable(name.constData(), QByteArrayView(env).sliced(idx + 1));
    }
}

void createConfigDirectory()
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (!QDir().mkpath(configDir))
        out << "Could not create config directory XDG_CONFIG_HOME: " << configDir << '\n';
}

void runStartupConfig()
{
    // export LC_* variables set by kcmshell5 formats into environment
    // so it can be picked up by QLocale and friends.
    KConfig config(QStringLiteral("plasma-localerc"));
    KConfigGroup formatsConfig = KConfigGroup(&config, QStringLiteral("Formats"));

    // Note: not all of these (e.g. LC_CTYPE) can currently be changed through system settings (but they can be changed by modifying
    // plasma-localrc manually).
    const auto lcValues = {"LANG",
                           "LC_ADDRESS",
                           "LC_COLLATE",
                           "LC_CTYPE",
                           "LC_IDENTIFICATION",
                           "LC_MONETARY",
                           "LC_MESSAGES",
                           "LC_MEASUREMENT",
                           "LC_NAME",
                           "LC_NUMERIC",
                           "LC_PAPER",
                           "LC_TELEPHONE",
                           "LC_TIME",
                           "LC_ALL"};
    for (auto lc : lcValues) {
        const QString value = formatsConfig.readEntry(lc, QString());
        if (!value.isEmpty()) {
            qputenv(lc, value.toUtf8());
        }
    }

    KConfigGroup languageConfig = KConfigGroup(&config, QStringLiteral("Translations"));
    const QString value = languageConfig.readEntry("LANGUAGE", QString());
    if (!value.isEmpty()) {
        qputenv("LANGUAGE", value.toUtf8());
    }

    if (!formatsConfig.hasKey("LANG") && !qEnvironmentVariableIsEmpty("LANG")) {
        formatsConfig.writeEntry("LANG", qgetenv("LANG"));
        formatsConfig.sync();
    }
}

void setupCursor(bool wayland)
{
#ifdef XCURSOR_PATH
    QByteArray path(XCURSOR_PATH);
    path.replace("$XCURSOR_PATH", qgetenv("XCURSOR_PATH"));
    qputenv("XCURSOR_PATH", path);
#endif

    // TODO: consider linking directly
    if (!wayland) {
        const KConfig cfg(QStringLiteral("kcminputrc"));
        const KConfigGroup inputCfg = cfg.group(QStringLiteral("Mouse"));

        const auto cursorTheme = inputCfg.readEntry("cursorTheme", QStringLiteral("breeze_cursors"));
        const auto cursorSize = inputCfg.readEntry("cursorSize", 24);

        runSync(QStringLiteral("kapplymousetheme"), {cursorTheme, QString::number(cursorSize)});
    }
}

std::optional<QProcessEnvironment> getSystemdEnvironment()
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                              QStringLiteral("/org/freedesktop/systemd1"),
                                              QStringLiteral("org.freedesktop.DBus.Properties"),
                                              QStringLiteral("Get"));
    msg << QStringLiteral("org.freedesktop.systemd1.Manager") << QStringLiteral("Environment");
    auto reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        return std::nullopt;
    }

    // Make sure the returned type is correct.
    auto arguments = reply.arguments();
    if (arguments.isEmpty() || arguments[0].userType() != qMetaTypeId<QDBusVariant>()) {
        return std::nullopt;
    }
    auto variant = qdbus_cast<QVariant>(arguments[0]);
    if (variant.typeId() != QMetaType::QStringList) {
        return std::nullopt;
    }

    const auto assignmentList = variant.toStringList();
    QProcessEnvironment ret;
    for (auto &env : assignmentList) {
        const int idx = env.indexOf(QLatin1Char('='));
        if (Q_LIKELY(idx > 0)) {
            ret.insert(env.left(idx), env.mid(idx + 1));
        }
    }

    return ret;
}

// Import systemd user environment.
//
// Systemd read ~/.config/environment.d which applies to all systemd user unit.
// But it won't work if plasma is not started by systemd.
void importSystemdEnvrionment()
{
    const auto environment = getSystemdEnvironment();
    if (!environment) {
        return;
    }

    const auto keys = environment.value().keys();
    for (const QString &nameStr : keys) {
        if (!isShellVariable(QStringView(nameStr)) && !isSessionVariable(nameStr)) {
            setEnvironmentVariable(nameStr.toLocal8Bit(), environment.value().value(nameStr).toLocal8Bit());
        }
    }
}

// Source scripts found in <config locations>/plasma-workspace/env/*.sh
// (where <config locations> correspond to the system and user's configuration
// directory.
//
// Scripts are sourced in reverse order of priority of their directory, as defined
// by `QStandardPaths::standardLocations`. This ensures that high-priority scripts
// (such as those in the user's home directory) are sourced last and take precedence
// over lower-priority scripts (such as system defaults). Scripts in the same
// directory are sourced in lexical order of their filename.
//
// This is where you can define environment variables that will be available to
// all KDE programs, so this is where you can run agents using e.g. eval `ssh-agent`
// or eval `gpg-agent --daemon`.
// Note: if you do that, you should also put "ssh-agent -k" as a shutdown script
//
// (see end of this file).
// For anything else (that doesn't set env vars, or that needs a window manager),
// better use the Autostart folder.

void runEnvironmentScripts()
{
    QStringList scripts;
    auto locations = QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation);

    //`standardLocations()` returns locations sorted by "order of priority". We iterate in reverse
    // order so that high-priority scripts are sourced last and their modifications take precedence.
    for (auto loc = locations.crbegin(); loc != locations.crend(); loc++) {
        QDir dir(*loc);
        if (!dir.cd(QStringLiteral("./plasma-workspace/env"))) {
            // Skip location if plasma-workspace/env subdirectory does not exist
            continue;
        }
        const auto dirScripts = dir.entryInfoList({QStringLiteral("*.sh")}, QDir::Files, QDir::Name);
        for (const auto &script : dirScripts) {
            scripts << script.absoluteFilePath();
        }
    }
    sourceFiles(scripts);
}

// Mark that full KDE session is running (e.g. Konqueror preloading works only
// with full KDE running). The KDE_FULL_SESSION property can be detected by
// any X client connected to the same X session, even if not launched
// directly from the KDE session but e.g. using "ssh -X", kdesu. $KDE_FULL_SESSION
// however guarantees that the application is launched in the same environment
// like the KDE session and that e.g. KDE utilities/libraries are available.
// KDE_FULL_SESSION property is also only available since KDE 3.5.5.
// The matching tests are:
//   For $KDE_FULL_SESSION:
//     if test -n "$KDE_FULL_SESSION"; then ... whatever
//   For KDE_FULL_SESSION property (on X11):
//     xprop -root | grep "^KDE_FULL_SESSION" >/dev/null 2>/dev/null
//     if test $? -eq 0; then ... whatever
//
// Additionally there is $KDE_SESSION_UID with the uid
// of the user running the KDE session. It should be rarely needed (e.g.
// after sudo to prevent desktop-wide functionality in the new user's kded).
//
// Since KDE4 there is also KDE_SESSION_VERSION, containing the major version number.
//

void setupPlasmaEnvironment()
{
    // Manually disable auto scaling because we are scaling above
    // otherwise apps that manually opt in for high DPI get auto scaled by the developer AND manually scaled by us
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");

    qputenv("KDE_FULL_SESSION", "true");
    qputenv("KDE_SESSION_VERSION", "6");
    qputenv("KDE_SESSION_UID", QByteArray::number(getuid()));
    qputenv("XDG_CURRENT_DESKTOP", "KDE");

    qputenv("KDE_APPLICATIONS_AS_SCOPE", "1");

    qputenv("XDG_MENU_PREFIX", "plasma-");

    // Add kdedefaults dir to allow config defaults overriding from a writable location
    QByteArray currentConfigDirs = qgetenv("XDG_CONFIG_DIRS");
    if (currentConfigDirs.isEmpty()) {
        currentConfigDirs = "/etc/xdg";
    }
    const QString extraConfigDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/kdedefaults");
    QDir().mkpath(extraConfigDir);
    qputenv("XDG_CONFIG_DIRS", QFile::encodeName(extraConfigDir) + ':' + currentConfigDirs);

    const KConfig globals;
    const QString currentLnf = KConfigGroup(&globals, QStringLiteral("KDE")).readEntry("LookAndFeelPackage", QStringLiteral("org.kde.breeze.desktop"));
    QFile activeLnf(extraConfigDir + QLatin1String("/package"));
    activeLnf.open(QIODevice::ReadOnly);
    if (activeLnf.readLine() != currentLnf.toUtf8()) {
        KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), currentLnf);
        LookAndFeelManager lnfManager;
        lnfManager.setMode(LookAndFeelManager::Mode::Defaults);
        lnfManager.save(package, KPackage::Package());
    }
    // check if colors changed, if so apply them and discard plasma cache
    {
        LookAndFeelManager lnfManager;
        lnfManager.setMode(LookAndFeelManager::Mode::Apply);
        KConfig globals(QStringLiteral("kdeglobals")); // Reload the config
        KConfigGroup generalGroup(&globals, QStringLiteral("General"));
        const QString colorScheme = generalGroup.readEntry("ColorScheme", QStringLiteral("BreezeLight"));
        QString path = lnfManager.colorSchemeFile(colorScheme);

        if (!path.isEmpty()) {
            QFile f(path);
            QCryptographicHash hash(QCryptographicHash::Sha1);
            if (f.open(QFile::ReadOnly) && hash.addData(&f)) {
                const QString fileHash = QString::fromUtf8(hash.result().toHex());
                if (fileHash != generalGroup.readEntry("ColorSchemeHash", QString())) {
                    lnfManager.setColors(colorScheme, path);
                    generalGroup.writeEntry("ColorSchemeHash", fileHash);
                    generalGroup.sync();
                    const QString svgCache =
                        QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1Char('/') + QStringLiteral("plasma-svgelements");
                    if (!svgCache.isEmpty()) {
                        QFile::remove(svgCache);
                    }
                }
            }
        }
    }
}

void cleanupPlasmaEnvironment(const std::optional<QProcessEnvironment> &oldSystemdEnvironment)
{
    qunsetenv("KDE_FULL_SESSION");
    qunsetenv("KDE_SESSION_VERSION");
    qunsetenv("KDE_SESSION_UID");

    if (!oldSystemdEnvironment) {
        return;
    }

    auto currentEnv = getSystemdEnvironment();
    if (!currentEnv) {
        return;
    }

    // According to systemd documentation:
    // If a variable is listed in both, the variable is set after this method returns, i.e. the set list overrides the unset list.
    // So this will effectively restore the state to the values in oldSystemdEnvironment.
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                          QStringLiteral("/org/freedesktop/systemd1"),
                                                          QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                          QStringLiteral("UnsetAndSetEnvironment"));
    message.setArguments({currentEnv.value().keys(), oldSystemdEnvironment.value().toStringList()});

    // The session program gonna quit soon, ensure the message is flushed.
    auto reply = QDBusConnection::sessionBus().asyncCall(message);
    reply.waitForFinished();
}

// Drop session-specific variables from the systemd environment.
// Those can be leftovers from previous sessions, which can interfere with the session
// we want to start now, e.g. $DISPLAY might break kwin_wayland.
static void dropSessionVarsFromSystemdEnvironment()
{
    const auto environment = getSystemdEnvironment();
    if (!environment) {
        return;
    }

    QStringList varsToDrop;
    const auto keys = environment.value().keys();
    for (const QString &nameStr : keys) {
        // If it's set in this process, it'll be overwritten by the following UpdateLaunchEnvJob
        if (!qEnvironmentVariableIsSet(nameStr.toLocal8Bit()) && isSessionVariable(nameStr)) {
            varsToDrop.append(nameStr);
        }
    }

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                              QStringLiteral("/org/freedesktop/systemd1"),
                                              QStringLiteral("org.freedesktop.systemd1.Manager"),
                                              QStringLiteral("UnsetEnvironment"));
    msg << varsToDrop;
    auto reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(PLASMA_STARTUP) << "Failed to unset systemd environment variables:" << reply.errorName() << reply.errorMessage();
    }
}

// kwin_wayland can possibly also start dbus-activated services which need env variables.
// In that case, the update in startplasma might be too late.
bool syncDBusEnvironment()
{
    dropSessionVarsFromSystemdEnvironment();

    // Shell variables are filtered out of things we explicitly load, but they
    // still might have been inherited from the parent process
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const auto keys = environment.keys();
    for (const QString &name : keys) {
        if (isShellVariable(QStringView(name))) {
            environment.remove(name);
        }
    }

    // At this point all environment variables are set, let's send it to the DBus session server to update the activation environment
    auto job = new KUpdateLaunchEnvironmentJob(environment);
    QEventLoop e;
    QObject::connect(job, &KUpdateLaunchEnvironmentJob::finished, &e, &QEventLoop::quit);
    e.exec();
    return true;
}

static bool desktopLockedAtStart = false;

QProcess *setupKSplash()
{
    const auto dlstr = qgetenv("DESKTOP_LOCKED");
    desktopLockedAtStart = dlstr == "true" || dlstr == "1";
    qunsetenv("DESKTOP_LOCKED"); // Don't want it in the environment

    QProcess *p = nullptr;
    if (!desktopLockedAtStart) {
        const KConfig cfg(QStringLiteral("ksplashrc"));
        // the splashscreen and progress indicator
        KConfigGroup ksplashCfg = cfg.group(QStringLiteral("KSplash"));
        if (ksplashCfg.readEntry("Engine", QStringLiteral("KSplashQML")) == QLatin1String("KSplashQML")) {
            p = new QProcess;
            p->setProcessChannelMode(QProcess::ForwardedChannels);
            p->start(QStringLiteral("ksplashqml"), {ksplashCfg.readEntry("Theme", QStringLiteral("Breeze"))});
        }
    }
    return p;
}

// If something went on an endless restart crash loop it will get blacklisted, as this is a clean login we will want to reset those counters
// This is independent of whether we use the Plasma systemd boot
void resetSystemdFailedUnits()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                          QStringLiteral("/org/freedesktop/systemd1"),
                                                          QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                          QStringLiteral("ResetFailed"));
    QDBusConnection::sessionBus().call(message);
}

// Reload systemd to make sure the current configuration is active, which also reruns generators.
// Needed for e.g. XDG autostart changes to become effective.
void reloadSystemd()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                          QStringLiteral("/org/freedesktop/systemd1"),
                                                          QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                          QStringLiteral("Reload"));
    QDBusConnection::sessionBus().call(message);
}

bool hasSystemdService(const QString &serviceName)
{
    qDBusRegisterMetaType<QPair<QString, QString>>();
    qDBusRegisterMetaType<QList<QPair<QString, QString>>>();
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                              QStringLiteral("/org/freedesktop/systemd1"),
                                              QStringLiteral("org.freedesktop.systemd1.Manager"),
                                              QStringLiteral("ListUnitFilesByPatterns"));
    msg << QStringList({QStringLiteral("enabled"), QStringLiteral("static"), QStringLiteral("linked"), QStringLiteral("linked-runtime")});
    msg << QStringList({serviceName});
    QDBusReply<QList<QPair<QString, QString>>> reply = QDBusConnection::sessionBus().call(msg);
    if (!reply.isValid()) {
        return false;
    }
    // if we have a service returned then it must have found it
    return !reply.value().isEmpty();
}

bool useSystemdBoot()
{
    auto config = KSharedConfig::openConfig(QStringLiteral("startkderc"), KConfig::NoGlobals);
    const QString configValue = config->group(QStringLiteral("General")).readEntry("systemdBoot", QStringLiteral("true")).toLower();

    if (configValue == QLatin1String("false")) {
        return false;
    }

    if (configValue == QLatin1String("force")) {
        qInfo() << "Systemd boot forced";
        return true;
    }

    if (!hasSystemdService(QStringLiteral("plasma-workspace.target"))) {
        return false;
    }

    // xdg-desktop-autostart.target is shipped with an systemd 246 and provides a generator
    // for creating units out of existing autostart files
    // only enable our systemd boot if that exists, unless the user has forced the systemd boot above
    return hasSystemdService(QStringLiteral("xdg-desktop-autostart.target"));
}

void startKSplashViaSystemd()
{
    const KConfig cfg(QStringLiteral("ksplashrc"));
    // the splashscreen and progress indicator
    KConfigGroup ksplashCfg = cfg.group(QStringLiteral("KSplash"));
    if (ksplashCfg.readEntry("Engine", QStringLiteral("KSplashQML")) == QLatin1String("KSplashQML")) {
        auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                  QStringLiteral("/org/freedesktop/systemd1"),
                                                  QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                  QStringLiteral("StartUnit"));
        msg << QStringLiteral("plasma-ksplash.service") << QStringLiteral("fail");
        QDBusReply<QDBusObjectPath> reply = QDBusConnection::sessionBus().call(msg);
    }
}

static void migrateUserScriptsAutostart()
{
    QDir configLocation(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
    QDir autostartScriptsLocation(configLocation.filePath(QStringLiteral("autostart-scripts")));
    if (!autostartScriptsLocation.exists()) {
        return;
    }
    const QDir autostartScriptsMovedLocation(configLocation.filePath(QStringLiteral("old-autostart-scripts")));
    const auto entries = autostartScriptsLocation.entryInfoList(QDir::Files);
    for (const auto &info : entries) {
        const auto scriptName = info.fileName();
        const auto scriptPath = info.absoluteFilePath();
        const auto scriptMovedPath = autostartScriptsMovedLocation.filePath(scriptName);

        // Don't migrate backup files
        if (scriptName.endsWith(QLatin1Char('~')) || scriptName.endsWith(QLatin1String(".bak"))
            || (scriptName[0] == QLatin1Char('%') && scriptName.endsWith(QLatin1Char('%')))
            || (scriptName[0] == QLatin1Char('#') && scriptName.endsWith(QLatin1Char('#')))) {
            qCDebug(PLASMA_STARTUP) << "Not migrating backup autostart script" << scriptName;
            continue;
        }

        // Migrate autostart script to a standard .desktop autostart file
        AutostartScriptDesktopFile desktopFile(scriptName,
                                               info.isSymLink() ? info.symLinkTarget() : scriptMovedPath,
                                               QStringLiteral("application-x-executable-script"));
        qCInfo(PLASMA_STARTUP) << "Migrated legacy autostart script" << scriptPath << "to" << desktopFile.fileName();

        if (info.isSymLink() && QFile::remove(scriptPath)) {
            qCInfo(PLASMA_STARTUP) << "Removed legacy autostart script" << scriptPath << "that pointed to" << info.symLinkTarget();
        }
    }
    // Delete or rename autostart-scripts to old-autostart-scripts to avoid running the migration again
    if (autostartScriptsLocation.entryInfoList(QDir::Files).empty()) {
        autostartScriptsLocation.removeRecursively();
    } else {
        configLocation.rename(autostartScriptsLocation.dirName(), autostartScriptsMovedLocation.dirName());
    }
    // Reload systemd so that the XDG autostart generator is run again to pick up the new .desktop files
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                          QStringLiteral("/org/freedesktop/systemd1"),
                                                          QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                          QStringLiteral("Reload"));
    QDBusConnection::sessionBus().call(message);
}

bool startPlasmaSession(bool wayland)
{
    resetSystemdFailedUnits();
    reloadSystemd();
    OrgKdeKSplashInterface iface(QStringLiteral("org.kde.KSplash"), QStringLiteral("/KSplash"), QDBusConnection::sessionBus());
    iface.setStage(QStringLiteral("startPlasma"));

    // this is used by plasma-sentinel to check when this procses exits and cleanup
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.startplasma"));

    // finally, give the session control to the session manager
    // see kdebase/ksmserver for the description of the rest of the startup sequence
    // if the KDEWM environment variable has been set, then it will be used as KDE's
    // window manager instead of kwin.
    // if KDEWM is not set, ksmserver will ensure kwin is started.
    // kwrapper5 is used to reduce startup time and memory usage
    // kwrapper5 does not return useful error codes such as the exit code of ksmserver.
    // We only check for 255 which means that the ksmserver process could not be
    // started, any problems thereafter, e.g. ksmserver failing to initialize,
    // will remain undetected.
    // If the session should be locked from the start (locked autologin),
    // lock now and do the rest of the KDE startup underneath the locker.

    bool rc = true;
    QEventLoop e;

    QDBusServiceWatcher serviceWatcher;
    serviceWatcher.setConnection(QDBusConnection::sessionBus());

    // We want to exit when both ksmserver and plasma-session-shutdown have finished
    // This also closes if ksmserver crashes unexpectedly, as in those cases plasma-shutdown is not running
    if (wayland) {
        serviceWatcher.addWatchedService(QStringLiteral("org.kde.KWinWrapper"));
    } else {
        serviceWatcher.addWatchedService(QStringLiteral("org.kde.ksmserver"));
    }
    serviceWatcher.addWatchedService(QStringLiteral("org.kde.Shutdown"));
    serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    QObject::connect(&serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, [&]() {
        const QStringList watchedServices = serviceWatcher.watchedServices();
        bool plasmaSessionRunning = std::any_of(watchedServices.constBegin(), watchedServices.constEnd(), [](const QString &service) {
            return QDBusConnection::sessionBus().interface()->isServiceRegistered(service);
        });
        if (!plasmaSessionRunning) {
            e.quit();
        }
    });

    // Create .desktop files for the scripts in .config/autostart-scripts
    migrateUserScriptsAutostart();

    std::unique_ptr<QProcess, KillBeforeDeleter> startPlasmaSession;
    if (!useSystemdBoot()) {
        startPlasmaSession.reset(new QProcess);
        qCDebug(PLASMA_STARTUP) << "Using classic boot";

        QStringList plasmaSessionOptions;
        if (wayland) {
            plasmaSessionOptions << QStringLiteral("--no-lockscreen");
        } else {
            if (desktopLockedAtStart) {
                plasmaSessionOptions << QStringLiteral("--lockscreen");
            }
        }

        startPlasmaSession->setProcessChannelMode(QProcess::ForwardedChannels);
        QObject::connect(startPlasmaSession.get(), &QProcess::finished, &e, [&rc](int exitCode, QProcess::ExitStatus) {
            if (exitCode == 255) {
                // Startup error
                messageBox(QStringLiteral("startkde: Could not start plasma_session. Check your installation.\n"));
                rc = false;
            }
        });

        startPlasmaSession->start(QStringLiteral(CMAKE_INSTALL_FULL_BINDIR "/plasma_session"), plasmaSessionOptions);
    } else {
        qCDebug(PLASMA_STARTUP) << "Using systemd boot";
        const QString platform = wayland ? QStringLiteral("wayland") : QStringLiteral("x11");

        auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                  QStringLiteral("/org/freedesktop/systemd1"),
                                                  QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                  QStringLiteral("StartUnit"));
        msg << QStringLiteral("plasma-workspace-%1.target").arg(platform) << QStringLiteral("fail");
        QDBusReply<QDBusObjectPath> reply = QDBusConnection::sessionBus().call(msg);
        if (!reply.isValid()) {
            qCWarning(PLASMA_STARTUP) << "Could not start systemd managed Plasma session:" << reply.error().name() << reply.error().message();
            messageBox(QStringLiteral("startkde: Could not start Plasma session.\n"));
            rc = false;
        } else {
            playStartupSound();
        }
        if (wayland) {
            startKSplashViaSystemd();
        }
    }
    if (rc) {
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, &e, &QEventLoop::quit);
        e.exec();
    }
    return rc;
}

void waitForKonqi()
{
    const KConfig cfg(QStringLiteral("startkderc"));
    const KConfigGroup grp = cfg.group(QStringLiteral("WaitForDrKonqi"));
    bool wait_drkonqi = grp.readEntry("Enabled", true);
    if (wait_drkonqi) {
        // wait for remaining drkonqi instances with timeout (in seconds)
        const int wait_drkonqi_timeout = grp.readEntry("Timeout", 900) * 1000;
        QElapsedTimer wait_drkonqi_counter;
        wait_drkonqi_counter.start();
        QStringList services = allServices(QLatin1String("org.kde.drkonqi-"));
        while (!services.isEmpty()) {
            sleep(5);
            services = allServices(QLatin1String("org.kde.drkonqi-"));
            if (wait_drkonqi_counter.elapsed() >= wait_drkonqi_timeout) {
                // ask remaining drkonqis to die in a graceful way
                for (const auto &service : std::as_const(services)) {
                    QDBusInterface iface(service, QStringLiteral("/MainApplication"));
                    iface.call(QStringLiteral("quit"));
                }
                break;
            }
        }
    }
}

namespace
{
void canberraFinishCallback(ca_context *c, uint32_t /*id*/, int error_code, void *userdata)
{
    QMetaObject::invokeMethod(
        qApp,
        [c, error_code] {
            if (error_code != CA_SUCCESS) {
                qCWarning(PLASMA_STARTUP) << "Failed to cancel canberra context for audio notification:" << ca_strerror(error_code);
            }
            ca_context_destroy(c);
        },
        Qt::QueuedConnection);
    // WARNING: do not do anything else here. If you need more logic then put it into a userdata object please.
    // WARNING: best invokeMethod into a QObject to ensure things run on the gui thread, we must not destroy in the callback!
}
} // namespace

void playStartupSound()
{
    // This features a bit of duplication from KNotifications. Why we need to do this stuff manually was never
    // documented, I am left guessing that it is this way so we don't end up talking to plasmashell while it is initing.

    KNotifyConfig notifyConfig(QStringLiteral("plasma_workspace"), QStringLiteral("startkde"));
    const QString action = notifyConfig.readEntry(QStringLiteral("Action"));
    if (action.isEmpty() || !action.split(QLatin1Char('|')).contains(QLatin1String("Sound"))) {
        // no startup sound configured
        return;
    }

    QString soundName = notifyConfig.readEntry(QStringLiteral("Sound"));
    if (soundName.isEmpty()) {
        qCWarning(PLASMA_STARTUP) << "Audio notification requested, but no sound file provided in notifyrc file, aborting audio notification";
        return;
    }

    const auto config = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    const KConfigGroup group = config->group(QStringLiteral("Sounds"));
    const auto soundTheme = group.readEntry("Theme", u"ocean"_s);
    if (!group.readEntry("Enable", true)) {
        qCDebug(PLASMA_STARTUP) << "Notification sounds are globally disabled";
        return;
    }

    // Legacy implementation. Fallback lookup for a full path within the `$XDG_DATA_LOCATION/sounds` dirs
    QUrl fallbackUrl;
    const auto dataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &dataLocation : dataLocations) {
        fallbackUrl = QUrl::fromUserInput(soundName, dataLocation + QStringLiteral("/sounds"), QUrl::AssumeLocalFile);
        if (fallbackUrl.isLocalFile() && QFileInfo::exists(fallbackUrl.toLocalFile())) {
            break;
        }
        if (!fallbackUrl.isLocalFile() && fallbackUrl.isValid()) {
            break;
        }
        fallbackUrl.clear();
    }

    ca_context *context = nullptr;
    // context dangles a bit. Gets cleaned up in the finish callback!
    if (int ret = ca_context_create(&context); ret != CA_SUCCESS) {
        qCWarning(PLASMA_STARTUP) << "Failed to initialize canberra context for startup sound:" << ca_strerror(ret);
        return;
    }

    // We aren't actually plasmashell, for all intents and purpose we are part of it though from the user perspective.
    if (int ret = ca_context_change_props(context,
                                          CA_PROP_APPLICATION_NAME,
                                          "plasmashell",
                                          CA_PROP_APPLICATION_ID,
                                          "org.kde.plasmashell.desktop",
                                          CA_PROP_APPLICATION_ICON_NAME,
                                          "plasmashell",
                                          nullptr);
        ret != CA_SUCCESS) {
        qCWarning(PLASMA_STARTUP) << "Failed to set application properties on canberra context for startup sound:" << ca_strerror(ret);
    }

    ca_proplist *props = nullptr;
    ca_proplist_create(&props);
    const auto proplistDestroy = qScopeGuard([props] {
        ca_proplist_destroy(props);
    });

    const QByteArray soundNameBytes = soundName.toUtf8();
    const QByteArray soundThemeBytes = soundTheme.toUtf8();
    const QByteArray fallbackUrlBytes = QFile::encodeName(fallbackUrl.toLocalFile());

    ca_proplist_sets(props, CA_PROP_EVENT_ID, soundNameBytes.constData());
    ca_proplist_sets(props, CA_PROP_CANBERRA_XDG_THEME_NAME, soundThemeBytes.constData());
    // Fallback to filename
    if (!fallbackUrl.isEmpty()) {
        ca_proplist_sets(props, CA_PROP_MEDIA_FILENAME, fallbackUrlBytes.constData());
    }
    // We only ever play this sound once, no need to cache it.
    ca_proplist_sets(props, CA_PROP_CANBERRA_CACHE_CONTROL, "never");

    if (int ret = ca_context_play_full(context, 0 /* id */, props, canberraFinishCallback, nullptr); ret != CA_SUCCESS) {
        qCWarning(PLASMA_STARTUP) << "Failed to play startup sound with canberra:" << ca_strerror(ret);
    }
}
