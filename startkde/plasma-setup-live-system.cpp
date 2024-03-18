// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2016 Jonathan Riddell <jr@jriddell.org>
// SPDX-FileCopyrightText: 2017-2024 Harald Sitter <sitter@kde.org>

#include <filesystem>

#include <QCoreApplication>
#include <QFile>

#include <KConfig>
#include <KConfigGroup>

using namespace Qt::StringLiterals;

struct System {
    System()
        : username([]() -> std::string {
            // Env variable always wins
            if (auto env = getenv("USERNAME"); env) {
                return env;
            }

            // In production we expect the kernel cmdline to carry the info
            QFile cmdlineFile(cmdlinePath());
            if (!cmdlineFile.open(QFile::ReadOnly)) {
                qFatal() << "Could not open cmdline @ " << cmdlinePath();
            }
            const auto cmdline = cmdlineFile.readLine();
            const auto entries = cmdline.split(' ');
            for (const auto &entry : entries) {
                if (!entry.startsWith("plasma.live.user=")) {
                    continue;
                }
                return entry.mid(entry.indexOf('=') + 1).toStdString();
            }

            // We are out of options
            qFatal() << "Please define plasma.live.user on the cmdline or USERNAME in environment";
            return "unknown-user";
        }())
        , sysroot([]() -> std::filesystem::path {
            if (auto env = getenv("SYSROOT"); env) {
                return env;
            }
            return "/";
        }())
        , xdg(sysroot / "etc/xdg")
    {
    }

    void disableUpdates()
    {
        std::filesystem::remove(xdg / "autostart/distro-release-notifier.desktop");
        std::filesystem::remove(xdg / "autostart/org.kde.discover.notifier.desktop");
    }

    [[nodiscard]] KConfig openConfig(const std::filesystem::path &path)
    {
        return KConfig(QString::fromStdString(path.string()), KConfig::SimpleConfig);
    }

    void disableScreenLocker()
    {
        auto config = openConfig(xdg / "kscreenlockerrc");
        auto group = config.group(u"Daemon"_s);
        group.writeEntry("Timeout", 0);
        group.writeEntry("Autolock", false);
    }

    void disableBaloo()
    {
        auto config = openConfig(xdg / "baloorc");
        auto group = config.group(u"Basic Settings"_s);
        group.writeEntry("Indexing-Enabled", false);
    }

    void disableKWallet()
    {
        // Prevents network manager from throwing up wallet related prompts.
        auto config = openConfig(xdg / "kwalletrc");
        auto group = config.group(u"Wallet"_s);
        group.writeEntry("Enabled", false);
    }

    void setupSDDM()
    {
        auto config = openConfig(sysroot / "etc/sddm.conf");
        auto group = config.group(u"Autologin"_s);
        group.writeEntry("User", username.c_str());
        group.writeEntry("Relogin", true);
        group.writeEntry("Session", "plasma.desktop");
    }

    void disableAutomounter()
    {
        // It can interfere with installers when they want to partition the automounted drives.
        auto config = openConfig(xdg / "kded_device_automounterrc");
        auto group = config.group(u"General"_s);
        group.writeEntry("AutomountEnabled", false);
        group.writeEntry("AutomountOnLogin", false);
        group.writeEntry("AutomountOnPlugin", false);
    }

private:
    [[nodiscard]] static QString cmdlinePath()
    {
        if (auto env = qEnvironmentVariable("CMDLINE"); !env.isEmpty()) {
            return env;
        }
        return u"/proc/cmdline"_s;
    }

    std::string username;
    std::filesystem::path sysroot;
    std::filesystem::path xdg;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    System system;
    system.disableBaloo();
    system.disableKWallet();
    system.disableScreenLocker();
    system.disableUpdates();
    system.setupSDDM();
}
