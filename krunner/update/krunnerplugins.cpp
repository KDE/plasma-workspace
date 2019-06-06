/***************************************************************************
 *   Copyright (C) 2016 by Marco Martin <mart@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QDir>

#include <KSharedConfig>
#include <KConfigGroup>
#include <kconfig.h>
#include <KPluginInfo>
#include <krunner/runnermanager.h>

void migrateEnabledPlugins()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig("krunnerrc");
    KConfigGroup runnerManagerGroup(config, "PlasmaRunnerManager");
    KConfigGroup pluginsGroup(config, "Plugins");

    QStringList enabledCategories = runnerManagerGroup.readEntry("enabledCategories", QStringList());

    if (enabledCategories.isEmpty()) {
        return;
    }

    Plasma::RunnerManager *manager = new Plasma::RunnerManager();
    manager->reloadConfiguration();
    for (Plasma::AbstractRunner *runner : manager->runners()) {
        pluginsGroup.writeEntry(runner->metadata().pluginName() + QStringLiteral("Enabled"), false);
        for (auto category : runner->categories()) {
            if (enabledCategories.contains(category)) {
                pluginsGroup.writeEntry(runner->metadata().pluginName() + QStringLiteral("Enabled"), true);
                break;
            }
        }
    }

    runnerManagerGroup.deleteEntry("enabledCategories");
}

int main(int argc, char **argv)
{

    QCoreApplication app(argc, argv);

    migrateEnabledPlugins();

    return 0;
}
