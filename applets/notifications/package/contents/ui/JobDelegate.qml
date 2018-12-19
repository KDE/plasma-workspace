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

    readonly property string infoMessage: getData(jobsSource.data, "infoMessage", '')
    readonly property string labelName0: getData(jobsSource.data, "labelName0", '')
    readonly property string labelName1: getData(jobsSource.data, "labelName1", '')
    readonly property string labelFileName0: getData(jobsSource.data, "labelFileName0", '')
    readonly property string labelFileName1: getData(jobsSource.data, "labelFileName1", '')
    readonly property string label0: getData(jobsSource.data, "label0", '')
    readonly property string label1: getData(jobsSource.data, "label1", '')
    readonly property bool isSuspended: getData(jobsSource.data, "state", '') === "suspended"

    property alias infoMessageVisible: infoLabel.visible

    function getData(data, name, defaultValue) {
        var source = model.name
        return data[source] ? (data[source][name] ? data[source][name] : defaultValue) : defaultValue;
    }

    PlasmaExtras.Heading {
        id: infoLabel
        width: parent.width
        opacity: 0.6
        level: 3
        text: jobItem.isSuspended ? i18nc("Placeholder is job name, eg. 'Copying'", "%1 (Paused)", infoMessage) : infoMessage
        textFormat: Text.PlainText
    }

    RowLayout {
        width: parent.width

        PlasmaComponents.Label {
            id: summary
            Layout.fillWidth: true
            elide: Text.ElideMiddle
            text: {
                var label = labelFileName1 || labelFileName0;
                var lastSlashIdx = label.lastIndexOf("/");
                return label.slice(lastSlashIdx + 1);
            }
            textFormat: Text.PlainText
        }

        PlasmaComponents.ToolButton {
            id: expandButton
            iconSource: checked ? "arrow-down" : (LayoutMirroring.enabled ? "arrow-left" : "arrow-right")
            tooltip: checked ? i18nc("A button tooltip; hides item details", "Hide Details")
                             : i18nc("A button tooltip; expands the item to show details", "Show Details")
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
            indeterminate: plasmoid.expanded && jobsSource.data[model.name]
                           && typeof jobsSource.data[model.name]["percentage"] === "undefined"
                           && !jobItem.isSuspended
        }

        PlasmaComponents.ToolButton {
            id: pauseButton
            iconSource: jobItem.isSuspended ? "media-playback-start" : "media-playback-pause"
            visible: getData(jobsSource.data, "suspendable", 0)

            onClicked: {
                var operationName = "suspend"
                if (jobItem.isSuspended) {
                    operationName = "resume"
                }
                var service = jobsSource.serviceForSource(model.name)
                var operation = service.operationDescription(operationName)
                service.startOperationCall(operation)
            }
        }

        PlasmaComponents.ToolButton {
            id: stopButton
            iconSource: "media-playback-stop"
            visible: getData(jobsSource.data, "killable", 0)

            onClicked: {
                var service = jobsSource.serviceForSource(model.name)
                var operation = service.operationDescription("stop")
                service.startOperationCall(operation)
            }
        }
    }

}
