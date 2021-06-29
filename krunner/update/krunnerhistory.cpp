/*
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
