/*
 *   Copyright 2012 Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Column {
    id: jobsRoot
    anchors {
        left: parent.left
        right: parent.right
    }

    property alias count: jobsRepeater.count
    //height: 192 // FIXME: should be dynamic, once childrenRect works again

    PlasmaCore.DataSource {
        id: jobsSource

        property variant runningJobs

        engine: "applicationjobs"
        interval: 0

        onSourceAdded: {
            connectSource(source);
        }

        onSourceRemoved: {
            if (!notifications) {
                return
            }

            var message = runningJobs[source]["label1"] ? runningJobs[source]["label1"] : runningJobs[source]["label0"]
            notifications.addNotification(
                source,
                runningJobs[source]["appIconName"],
                0,
                runningJobs[source]["appName"],
                i18n("%1 [Finished]", runningJobs[source]["infoMessage"]),
                message,
                true,
                6000,
                0,
                0,
                0,
                [{"id": message, "text": i18n("Open")}])

            delete runningJobs[source]
        }

        onNewData: {
            var jobs = runningJobs
            jobs[sourceName] = data
            runningJobs = jobs
        }

        onDataChanged: {
            var total = 0
            for (var i = 0; i < sources.length; ++i) {
                if (jobsSource.data[sources[i]]["percentage"]) {
                    total += jobsSource.data[sources[i]]["percentage"]
                }
            }

            total /= sources.length
            notificationsApplet.globalProgress = total/100
        }

        Component.onCompleted: {
            jobsSource.runningJobs = new Object
            connectedSources = sources
        }
    }

    Title {
        visible: jobsRepeater.count > 0 && notifications && notifications.count > 0
        text: i18n("Transfers")
    }

    Item {
        visible: jobsRepeater.count > 3

        PlasmaComponents.ProgressBar {
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                right: parent.right
            }

            minimumValue: 0
            maximumValue: 100
            value: notificationsApplet.globalProgress * 100
        }
    }

    Repeater {
        id: jobsRepeater

        model: jobsSource.sources
        delegate: JobDelegate {
            toolIconSize: notificationsApplet.toolIconSize
        }
    }
}