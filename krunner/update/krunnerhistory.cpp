/***************************************************************************
 *   Copyright (C) 2020 by Alexander Lohnau <alexander.lohnau@gmx.de>      *
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
#include <QTimer>

#include <KActivities/Consumer>
#include <KConfigGroup>
#include <KSharedConfig>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    // Migrate data to state data file
    KSharedConfigPtr krunnerrc = KSharedConfig::openConfig("krunnerrc");
    KConfigGroup stateData = krunnerrc->group("PlasmaRunnerManager");
    KSharedConfigPtr newStateLocation = KSharedConfig::openConfig("krunnerstaterc", KConfig::NoGlobals, QStandardPaths::GenericDataLocation);
    stateData.reparent(newStateLocation.data());
    stateData.sync();

    // Migrate history to activity aware config
    auto consumer = new KActivities::Consumer();
    // Wait a bit for consumer to be initialized
    QObject::connect(consumer, &KActivities::Consumer::serviceStatusChanged, consumer, [consumer, newStateLocation, krunnerrc]() {
        const QString history = krunnerrc->group("General").readEntry("history");
        QStringList activities = consumer->activities();
        if (activities.isEmpty()) {
            activities.append(QStringLiteral("00000000-0000-0000-0000-000000000000"));
        }
        KConfigGroup newHistory = newStateLocation->group("PlasmaRunnerManager").group("History");
        for (const QString &activity : qAsConst(activities)) {
            newHistory.writeEntry(activity, history);
        }
        newHistory.sync();
        // Delete old values
        krunnerrc->group("General").deleteEntry("history");
        krunnerrc->deleteGroup("PlasmaRunnerManager");
        krunnerrc->group("PlasmaRunnerManager").writeEntry("migrated", true);
        krunnerrc->sync();
        qApp->exit();
    });

    return QCoreApplication::exec();
}
