/*
 *   Copyright 2011 Marco Martin <notmart@gmail.com>
 *   Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
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
import QtQuick.Layouts 1.1

import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0

Column {
    id: jobItem

    width: parent.width
    spacing: jobItem.layoutSpacing

    readonly property int layoutSpacing: units.largeSpacing / 4
    readonly property int animationDuration: units.shortDuration * 2

    readonly property string labelName0: getData(jobsSource.data, "labelName0", '')
    readonly property string label0: getData(jobsSource.data, "label0", '')
    readonly property string jobstate: getData(jobsSource.data, "state", '')

    function getData(data, name, defaultValue) {
        return data[modelData] ? (data[modelData][name] ? data[modelData][name] : defaultValue) : defaultValue;
    }

    PlasmaExtras.Heading {
        id: infoLabel
        width: parent.width
        opacity: 0.6
        level: 3
        text: getData(jobsSource.data, "infoMessage", '')
    }

    RowLayout {
        width: parent.width

        PlasmaComponents.Label {
            id: summary
            Layout.fillWidth: true
            elide: Text.ElideMiddle
            text: {
                var labelSplit = label0.split("/")
                return labelSplit[labelSplit.length-1]
            }
        }

        PlasmaComponents.ToolButton {
            id: expandButton
            iconSource: checked ? "list-remove" : "list-add"
            checkable: true
            onCheckedChanged: {
                if (checked) {
                    // Need to force the Loader active here, otherwise the transition
                    // doesn't fire because the height is still 0 without a loaded item
                    detailsLoader.active = true
                }
            }
        }
    }

    Loader {
        id: detailsLoader
        width: parent.width
        height: 0
        //visible: false // this breaks the opening transition but given the loaded item is released anyway...
        source: "JobDetailsItem.qml"
        active: false
        opacity: state === "expanded" ? 0.6 : 0
        Behavior on opacity { NumberAnimation { duration: jobItem.animationDuration } }

        states: [
            State {
                name: "expanded"
                when: expandButton.checked && detailsLoader.status === Loader.Ready
                PropertyChanges {
                    target: detailsLoader
                    height: detailsLoader.item.implicitHeight
                }
            }
        ]
        transitions : [
            Transition {
                from: "" // default state - collapsed
                to: "expanded"
                SequentialAnimation {
                    ScriptAction {
                        script: detailsLoader.clip = true
                    }
                    NumberAnimation {
                        duration: jobItem.animationDuration
                        properties: "height"
                        easing.type: Easing.InOutQuad
                    }
                    ScriptAction { script: detailsLoader.clip = false }
                }
            },
            Transition {
                from: "expanded"
                to: "" // default state - collapsed
                SequentialAnimation {
                    ScriptAction { script: detailsLoader.clip = true }
                    NumberAnimation {
                        duration: jobItem.animationDuration
                        properties: "height"
                        easing.type: Easing.InOutQuad
                    }
                    ScriptAction {
                        script: {
                            detailsLoader.clip = false
                            detailsLoader.active = false
                        }
                    }
                }
            }
        ]
    }

    RowLayout {
        width: parent.width
        height: pauseButton.height
        spacing: jobItem.layoutSpacing

        PlasmaComponents.ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            //height: units.gridUnit
            minimumValue: 0
            maximumValue: 100
            //percentage doesn't always exist, so doesn't get in the model
            value: getData(jobsSource.data, "percentage", 0)
            indeterminate: jobsSource.data[modelData] && typeof jobsSource.data[modelData]["percentage"] === "undefined"
        }

        PlasmaComponents.ToolButton {
            id: pauseButton
            iconSource: jobItem.jobstate == "suspended" ? "media-playback-start" : "media-playback-pause"

            onClicked: {
                var operationName = "suspend"
                if (jobItem.jobstate == "suspended") {
                    operationName = "resume"
                }
                var service = jobsSource.serviceForSource(modelData)
                var operation = service.operationDescription(operationName)
                service.startOperationCall(operation)
            }
        }

        PlasmaComponents.ToolButton {
            id: stopButton
            iconSource: "media-playback-stop"

            onClicked: {
                cancelledJobs.push(modelData) // register that it was user-cancelled
                var service = jobsSource.serviceForSource(modelData)
                var operation = service.operationDescription("stop")
                service.startOperationCall(operation)
            }
        }
    }

}
