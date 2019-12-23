/*
 *  Copyright 2013 Bhushan Shah <bhush94@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.5
import QtQuick.Controls 2.5 as QQC2
import QtQuick.Layouts 1.3

import org.kde.kirigami 2.5 as Kirigami
import org.kde.plasma.core 2.0 as PlasmaCore


Item {
    id: iconsPage
    width: childrenRect.width
    height: childrenRect.height
    implicitWidth: formLayout.implicitWidth
    implicitHeight: formLayout.implicitHeight

    property var cfg_sources: []

    function sourcesChanged() {
        if (! cfg_sources) { cfg_sources = [] }
        if (cfg_sources.length == 0) {
            for (var i in dataSourcesColumn.children) {
                var child = dataSourcesColumn.children[i];
                if (child.checked !== undefined) {
                    child.checked = sourceDefaultEnable(child.source);
                }
            }
        } else {
            for (var i in dataSourcesColumn.children) {
                var child = dataSourcesColumn.children[i];
                if (child.checked !== undefined) {
                    child.checked = cfg_sources.indexOf(child.source) !== -1;
                }
            }
        }
    }

    function sourceDefaultEnable(source) {
        return true;
    }

    onCfg_sourcesChanged: {
        sourcesChanged();
    }

    property int cfg_updateInterval

    signal sourceAdded(string source)

    function addSource(source, friendlyName) {
        var found = false;
        for (var i = 0; i < sourcesModel.count; ++i) {
            var obj = sourcesModel.get(i);
            if (obj.source === source) {
                found = true;
                break;
            }
        }
        if (found) {
            return;
        }

        sourcesModel.append(
                {"source": encodeURIComponent(source),
                 "friendlyName": friendlyName});
    }

    PlasmaCore.DataSource {
        id: smSource

        engine: "systemmonitor"

        onSourceAdded: {
            iconsPage.sourceAdded(source);
        }
        onSourceRemoved: {
            for (var i = sourcesModel.count - 1; i >= 0; --i) {
                var obj = sourcesModel.get(i);
                if (obj.source === source) {
                    sourcesModel.remove(i);
                }
            }
        }
    }

    Component.onCompleted: {
        for (var i in smSource.sources) {
            var source = smSource.sources[i];
            iconsPage.sourceAdded(source);
        }
        sourcesChanged();
    }

    ListModel {
        id: sourcesModel
    }

    Kirigami.FormLayout {
        id: formLayout

        anchors.left: parent.left
        anchors.right: parent.right

        QQC2.SpinBox {
            id: updateIntervalSpinBox
            Kirigami.FormData.label: i18n("Update interval:")
            from: 100
            stepSize: 100
            to: 1000000
            editable: true
            validator: DoubleValidator {
                bottom: spinbox.from
                top: spinbox.to
            }
            textFromValue: function(value) {
                var seconds = value / 1000
                return i18ncp("SpinBox text", "%1 second", "%1 seconds", seconds.toFixed(1))
            }
            valueFromText: function(text) {
                return parseFloat(text) * 1000
            }
            value: cfg_updateInterval
            onValueModified: cfg_updateInterval = value
        }

        Item {
            Kirigami.FormData.isSection: true
        }


        ColumnLayout {
            id: dataSourcesColumn
            Kirigami.FormData.label: i18n("Show:")
            Kirigami.FormData.buddyFor: children[1] // 0 is the Repeater

            Repeater {
                id: repeater
                model: sourcesModel
                QQC2.CheckBox {
                    id: checkBox
                    text: model.friendlyName
                    property string source: model.source
                    onCheckedChanged: {
                        if (checked) {
                            if (cfg_sources.indexOf(model.source) == -1) {
                                cfg_sources.push(model.source);
                            }
                        } else {
                            var idx = cfg_sources.indexOf(model.source);
                            if (cfg_sources.length !== 1) { // This condition prohibits turning off the last item from the list.
                                if (idx !== -1) {
                                    cfg_sources.splice(idx, 1);
                                }
                            }
                        }
                        cfg_sourcesChanged();
                    }
                }
            }
        }
    }
}
