    /* This file is part of the KDE project
   Copyright (C) 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kcheckrunning/kcheckrunning.h"
#include "kstartupconfig/kstartupconfig.h"
#include <ksplashinterface.h>

#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <KSharedConfig>
#include <KConfigGroup>

#include <signal.h>
#include <unistd.h>

QTextStream out(stderr);

QStringList allServices(const QLatin1String& prefix)
{
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    const QStringList services = bus->registeredServiceNames();
    QMap<QString, QStringList> servicesWithAliases;

    for (const QString &serviceName : services) {
        QDBusReply<QString> reply = bus->serviceOwner(serviceName);
        QString owner = reply;
        if (owner.isEmpty())
            owner = serviceName;
        servicesWithAliases[owner].append(serviceName);
    }

    QStringList names;
    for (auto it = servicesWithAliases.constBegin(); it != servicesWithAliases.constEnd(); ++it) {
        if (it.value().startsWith(prefix))
            names << it.value();
    }
    names.removeDuplicates();
    names.sort();
    return names;
}

int runSync(const QString& program, const QStringList &args, const QStringList &env = {})
{
    QProcess p;
    p.setEnvironment(env);
    p.start(program, args);
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.waitForFinished();
    return p.exitCode();
}

void messageBox(const QString &text)
{
    out << text;
    runSync("xmessage", {"-geometry" "500x100", text});
}

void writeFile(const QString& path, const QByteArray& contents)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        out << "Could not write into " << f.fileName() <<".\n";
        exit(1);
    }
    f.write(contents);
}

void sourceFiles(const QStringList &files)
{
    QStringList filteredFiles;
    std::copy_if(files.begin(), files.end(), std::back_inserter(filteredFiles), [](const QString& i){ return QFileInfo(i).isReadable(); } );

    if (filteredFiles.isEmpty())
        return;

    const QStringList args = QStringList(CMAKE_INSTALL_FULL_LIBEXECDIR "/plasma-sourceenv.sh") << filteredFiles;

    QProcess p;
    p.start("/bin/sh", args);
    p.waitForFinished();

    const auto fullEnv = p.readAllStandardOutput();
    auto envs = fullEnv.split('\n');
    for (auto &env: envs) {
        putenv(env.data());
    }
}

void sighupHandler(int)
{
    out << "GOT SIGHUP\n";
}

int main(int /*argc*/, char** /*argv*/)
{
    // When the X server dies we get a HUP signal from xinit. We must ignore it
    // because we still need to do some cleanup.
    signal(SIGHUP, sighupHandler);

    // Boot sequence:
    //
    // kdeinit is used to fork off processes which improves memory usage
    // and startup time.
    //
    // * kdeinit starts klauncher first.
    // * Then kded is started. kded is responsible for keeping the sycoca
    // database up to date. When an up to date database is present it goes
    // into the background and the startup continues.
    // * Then kdeinit starts kcminit. kcminit performs initialisation of
    // certain devices according to the user's settings
    //
    // * Then ksmserver is started which takes control of the rest of the startup sequence

    // Check if a Plasma session already is running and whether it's possible to connect to X
    switch (kCheckRunning()) {
        case NoX11:
            out << "$DISPLAY is not set or cannot connect to the X server.\n";
            return 1;
        case PlasmaRunning:
            messageBox("Plasma seems to be already running on this display.\n");
            return 1;
        case NoPlasmaRunning:
            break;
    }

    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (!QDir().mkpath(configDir))
        out << "Could not create config directory XDG_CONFIG_HOME: " << configDir << '\n';

    //This is basically setting defaults so we can use them with kStartupConfig()
    //TODO: see into passing them as an argument
    writeFile(configDir + "/startupconfigkeys",
        "kcminputrc Mouse cursorTheme 'breeze_cursors'\n"
        "kcminputrc Mouse cursorSize ''\n"
        "ksplashrc KSplash Theme Breeze\n"
        "ksplashrc KSplash Engine KSplashQML\n"
        "kdeglobals KScreen ScaleFactor ''\n"
        "kdeglobals KScreen ScreenScaleFactors ''\n"
        "kcmfonts General forceFontDPI 0\n"
    );

    //preload the user's locale on first start
    const QString localeFile = configDir + "/plasma-localerc";
    if (!QFile::exists(localeFile)) {
        writeFile(localeFile,
            QByteArray("[Formats]\n"
            "LANG=" +qgetenv("LANG")+ "\n"));
    }

    if (int code = kStartupConfig()) {
        messageBox("kStartupConfig() does not exist or fails. The error code is " + QByteArray::number(code) + ". Check your installation.\n");
        return 1;
    }

    //export LC_* variables set by kcmshell5 formats into environment
    //so it can be picked up by QLocale and friends.
    sourceFiles({configDir + "/startupconfig", configDir + "/plasma-locale-settings.sh"});

#if !defined(WAYLAND)
    //Do not sync any of this section with the wayland versions as there scale factors are
    //sent properly over wl_output

    {
        const auto screenScaleFactors = qgetenv("kdeglobals_kscreen_screenscalefactors");
        if (!screenScaleFactors.isEmpty()) {
            qputenv("QT_SCREEN_SCALE_FACTORS", screenScaleFactors);
            if (screenScaleFactors == "2" || screenScaleFactors == "3") {
                qputenv("GDK_SCALE", screenScaleFactors);
                qputenv("GDK_DPI_SCALE", QByteArray::number(1/screenScaleFactors.toInt(), 'g', 3));
            }
        }
    }
#endif

    //Manually disable auto scaling because we are scaling above
    //otherwise apps that manually opt in for high DPI get auto scaled by the developer AND manually scaled by us
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");

    //XCursor mouse theme needs to be applied here to work even for kded or ksmserver
    {
        const auto kcminputrc_mouse_cursorsize = qgetenv("kcminputrc_mouse_cursorsize");
        const auto kcminputrc_mouse_cursortheme = qgetenv("kcminputrc_mouse_cursortheme");
        if (!kcminputrc_mouse_cursortheme.isEmpty() || !kcminputrc_mouse_cursorsize.isEmpty()) {
#ifdef XCURSOR_PATH
            QByteArray path(XCURSOR_PATH);
            path.replace("$XCURSOR_PATH", qgetenv("XCURSOR_PATH"));
            qputenv("XCURSOR_PATH", path);
#endif

            //TODO: consider linking directly
            if (runSync("kapplymousetheme", { "kcminputrc_mouse_cursortheme", "kcminputrc_mouse_cursorsize" }) == 10) {
                qputenv("XCURSOR_THEME", "breeze_cursors");
            } else if (!kcminputrc_mouse_cursortheme.isEmpty()) {
                qputenv("XCURSOR_THEME", kcminputrc_mouse_cursortheme);
            }
            if (!kcminputrc_mouse_cursorsize.isEmpty()) {
                qputenv("XCURSOR_SIZE", kcminputrc_mouse_cursorsize);
            }
        }
    }

    {
        const auto kcmfonts_general_forcefontdpi = qgetenv("kcmfonts_general_forcefontdpi");
        if (kcmfonts_general_forcefontdpi != "0") {
            const QByteArray input = "Xft.dpi: kcmfonts_general_forcefontdpi";
            runSync("xrdb", { "-quiet", "-merge", "-nocpp" });
        }
    }

    QScopedPointer<QProcess> ksplash;
    const int dl = qEnvironmentVariableIntValue("DESKTOP_LOCKED");
    {
        qunsetenv("DESKTOP_LOCKED"); // Don't want it in the environment

        if (dl) {
            const auto ksplashrc_ksplash_engine = qgetenv("ksplashrc_ksplash_engine");
            // the splashscreen and progress indicator
            if (ksplashrc_ksplash_engine == "KSplashQML") {
                ksplash.reset(new QProcess());
                ksplash->start("ksplashqml", {QString::fromUtf8(qgetenv("ksplashrc_ksplash_theme"))});
            }
        }
    }


    // Source scripts found in <config locations>/plasma-workspace/env/*.sh
    // (where <config locations> correspond to the system and user's configuration
    // directory.
    //
    // This is where you can define environment variables that will be available to
    // all KDE programs, so this is where you can run agents using e.g. eval `ssh-agent`
    // or eval `gpg-agent --daemon`.
    // Note: if you do that, you should also put "ssh-agent -k" as a shutdown script
    //
    // (see end of this file).
    // For anything else (that doesn't set env vars, or that needs a window manager),
    // better use the Autostart folder.

    {
        QStringList scripts;
        const auto locations = QStandardPaths::locateAll(QStandardPaths::GenericConfigLocation, QStringLiteral("plasma-workspace/env"), QStandardPaths::LocateDirectory);
        for (const QString & location : locations) {
            QDir dir(location);
            const auto dirScripts = dir.entryInfoList({QStringLiteral("*.sh")});
            for (const auto script : dirScripts) {
                scripts << script.absoluteFilePath();
            }
        }
        sourceFiles(scripts);
    }

//     Set a left cursor instead of the standard X11 "X" cursor, since I've heard
//     from some users that they're confused and don't know what to do. This is
//     especially necessary on slow machines, where starting KDE takes one or two
//     minutes until anything appears on the screen.
//
//     If the user has overwritten fonts, the cursor font may be different now
//     so don't move this up.

    runSync("xsetroot", {"-cursor_name", "left_ptr"});

    // Get Ghostscript to look into user's KDE fonts dir for additional Fontmap
    {
        const QByteArray usr_fdir = QFile::encodeName(QDir::home().absoluteFilePath(".fonts"));
        if (qEnvironmentVariableIsSet("GS_LIB")) {
            qputenv("GS_LIB", usr_fdir + ':' + qgetenv("GS_LIB"));
        } else {
            qputenv("GS_LIB", usr_fdir);
        }
    }

    out << "startkde: Starting up...\n";

    // Make sure that the KDE prefix is first in XDG_DATA_DIRS and that it's set at all.
    // The spec allows XDG_DATA_DIRS to be not set, but X session startup scripts tend
    // to set it to a list of paths *not* including the KDE prefix if it's not /usr or
    // /usr/local.
    if (!qEnvironmentVariableIsSet("XDG_DATA_DIRS")) {
        qputenv("XDG_DATA_DIRS", KDE_INSTALL_FULL_DATAROOTDIR ":/usr/share:/usr/local/share");
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
    //   For KDE_FULL_SESSION property:
    //     xprop -root | grep "^KDE_FULL_SESSION" >/dev/null 2>/dev/null
    //     if test $? -eq 0; then ... whatever
    //
    // Additionally there is (since KDE 3.5.7) $KDE_SESSION_UID with the uid
    // of the user running the KDE session. It should be rarely needed (e.g.
    // after sudo to prevent desktop-wide functionality in the new user's kded).
    //
    // Since KDE4 there is also KDE_SESSION_VERSION, containing the major version number.
    // Note that this didn't exist in KDE3, which can be detected by its absense and
    // the presence of KDE_FULL_SESSION.

    qputenv("KDE_FULL_SESSION", "true");
    runSync("xprop", {"-root", "-f", "KDE_FULL_SESSION", "8t", "-set", "KDE_FULL_SESSION", "true"});

    qputenv("KDE_SESSION_VERSION", "5");
    runSync("xprop", {"-root", "-f", "KDE_SESSION_VERSION", "32c", "-set", "KDE_SESSION_VERSION", "5"});

    qputenv("KDE_SESSION_UID", QByteArray::number(getuid()));
    qputenv("XDG_CURRENT_DESKTOP", "KDE");

    {
        int exitCode;
        // At this point all environment variables are set, let's send it to the DBus session server to update the activation environment
        if (!QStandardPaths::findExecutable("dbus-update-activation-environment").isEmpty())
            exitCode = runSync("dbus-update-activation-environment", { "--systemd", "--all" });
        else
            exitCode = runSync(CMAKE_INSTALL_FULL_LIBEXECDIR "/ksyncdbusenv", {});

        if (exitCode != 0) {
            // Startup error
            if (ksplash)
                ksplash->kill();
            messageBox("Could not sync environment to dbus.\n");
            return 1;
        }
    }

    {
        // We set LD_BIND_NOW to increase the efficiency of kdeinit.
        // kdeinit unsets this variable before loading applications.
        const int exitCode = runSync(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/start_kdeinit_wrapper", { "--kded", "+kcminit_startup" }, { "LD_BIND_NOW=true" });
        if (exitCode != 0) {
            if (ksplash)
                ksplash->kill();
            messageBox("startkde: Could not start kdeinit5. Check your installation.");
            return 1;
        }
    }

    {
        OrgKdeKSplashInterface iface("org.kde.KSplash", "/KSplash", QDBusConnection::sessionBus());
        iface.setStage("kinit");
    }

    // finally, give the session control to the session manager
    // see plasma-workspace/ksmserver for the description of the rest of the startup sequence
    // if the KDEWM environment variable has been set, then it will be used as KDE's
    // window manager instead of kwin.
    // if KDEWM is not set, ksmserver will ensure kwin is started.
    // kwrapper5 is used to reduce startup time and memory usage
    // kwrapper5 does not return useful error codes such as the exit code of ksmserver.
    // We only check for 255 which means that the ksmserver process could not be
    // started, any problems thereafter, e.g. ksmserver failing to initialize,
    // will remain undetected.
    // st -n "$KDEWM" && KDEWM="--windowmanager $KDEWM"
    // If the session should be locked from the start (locked autologin),
    // lock now and do the rest of the KDE startup underneath the locker.
    QStringList ksmserverOptions(CMAKE_INSTALL_FULL_BINDIR "/ksmserver");
    if (qEnvironmentVariableIsSet("KDEWM"))
        ksmserverOptions << qEnvironmentVariable("KDEWM");

    if (dl) {
        ksmserverOptions << "--lockscreen";
    }
    const auto exitCode = runSync("kwrapper5", ksmserverOptions);

    if (exitCode == 255) {
        // Startup error
        if (ksplash)
            ksplash->kill();
        messageBox("startkde: Could not start ksmserver. Check your installation.\n");
        return 1;
    }

    // Anything after here is logout
    // It is not called after shutdown/restart

    {
        const auto cfg = KSharedConfig::openConfig("startkderc");
        KConfigGroup grp(cfg, "WaitForDrKonqi");
        bool wait_drkonqi =  grp.readEntry("Enabled", true);
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
                    for (const auto &service: services) {
                        QDBusInterface iface(service, "/MainApplication");
                        iface.call("quit");
                    }
                    break;
                }
            }
        }
    }

    out << "startkde: Shutting down...\n";

    // just in case
    if (ksplash)
        ksplash->kill();

    runSync("kdeinit5_shutdown", {});

    qunsetenv("KDE_FULL_SESSION");
    runSync("xprop", { "-root", "-remove", "KDE_FULL_SESSION" });
    qunsetenv("KDE_SESSION_VERSION");
    runSync("xprop", { "-root", "-remove", "KDE_SESSION_VERSION" });
    qunsetenv("KDE_SESSION_UID");

    out << "startkde: Done.\n";

    return 0;
}
