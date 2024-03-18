// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2016 Jonathan Riddell <jr@jriddell.org>
// SPDX-FileCopyrightText: 2017-2024 Harald Sitter <sitter@kde.org>

#include <filesystem>

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#include <KConfig>
#include <KConfigGroup>
#include <KDesktopFile>

using namespace Qt::StringLiterals;

struct User {
    User()
        : sysroot([]() -> std::filesystem::path {
            if (auto env = getenv("SYSROOT"); env) {
                return env;
            }
            return "/";
        }())
        , installers({
              {Installer::Calamares, sysroot / "usr/share/applications/calamares.desktop"}, //
              {Installer::Anaconda, sysroot / "usr/share/applications/liveinst.desktop"}, //
          })
        , installer([this]() -> Installer {
            for (const auto &[installer, path] : installers) {
                if (std::filesystem::exists(path)) {
                    return installer;
                }
            }
            return Installer::Unknown;
        }())
    {
    }

    void populateHome() const
    {
        if (installer != Installer::Unknown) {
            const auto &installerDesktopFile = installers.at(installer);
            auto qDesktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
            auto desktop = std::filesystem::path(qDesktop.toStdString());
            if (!std::filesystem::create_directories(desktop)) {
                qWarning() << "Failed to create" << desktop.string();
            }
            const auto targetPath = desktop / installerDesktopFile.filename();
            if (!std::filesystem::copy_file(installerDesktopFile, desktop / installerDesktopFile.filename())) {
                qWarning() << "Failed to copy" << installerDesktopFile.string() << "to" << desktop.string();
            }
            std::filesystem::permissions(targetPath, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
            KDesktopFile desktopFile(QString::fromStdString(targetPath));
            desktopFile.desktopGroup().writeEntry("NoDisplay", false);
        }
    }

    void setupPlasmaWelcome() const
    {
        auto config = KConfig(u"plasma-welcomerc"_s);
        auto group = config.group(u"General"_s);
        group.writeEntry("LiveEnvironment", true);
        switch (installer) {
        case Installer::Unknown:
            break;
        case Installer::Anaconda:
            group.writeEntry("LiveInstaller", "liveinst");
            break;
        case Installer::Calamares:
            group.writeEntry("LiveInstaller", "calamares");
            break;
        }
    }

private:
    enum class Installer {
        Unknown,
        Calamares,
        Anaconda,
    };
    std::filesystem::path sysroot;
    std::map<Installer, std::filesystem::path> installers;
    Installer installer;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    User user;
    user.populateHome();
    user.setupPlasmaWelcome();
}
