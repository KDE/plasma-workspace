/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

.pragma library

var items = new Array()
var tasksRow
var appletsFlickableParent
var plasmoid
var root

function insertAt(item, index)
{
    remove(item)
    if (index < 0 || index > (items.length-1)) {
        items.push(item)
        item.parent = tasksRow
        return
    }

    //reinsert at old position? do nothing
    if (item == items[index]) {
        return;
    }

    var oldChildren = Array()
    for (var i = index; i < items.length; ++i) {
        oldChildren[oldChildren.length] = items[i]
    }

    item.parent = appletsFlickableParent
    for (var i = 0; i < oldChildren.length; ++i) {
        oldChildren[i].parent = appletsFlickableParent
    }
    item.parent = tasksRow
    for (var i = 0; i < oldChildren.length; ++i) {
        oldChildren[i].parent = tasksRow
    }
    items.splice(index, 0, item)
}

function remove(item)
{
    var index = -1
    for (var i = 0; i < items.length; ++i) {
        if (items[i] == item) {
            item.parent = appletsFlickableParent
            index = i
            break
        }
    }

    if (index >= 0) {
        items.splice(index, 1)
    }
}

function saveOrder()
{
    var order = String()
    for (var i = 0; i < items.length; ++i) {
        if (items[i].applet.id >= 0) {
            order += ":" + items[i].applet.id
        }
    }
    plasmoid.configuration.AppletsOrder = order
}

function restoreOrder()
{
        var appletsOrder = String(plasmoid.configuration.AppletsOrder)
        //array with all the applet ids, in order
        var appletIds = Array()
        if (appletsOrder.length > 0) {
            appletIds = appletsOrder.split(":")
        }

        //all applets loaded, indicized by id
        var appletsForId = new Array()

        //fill appletsForId
        for (var i = 0; i < plasmoid.applets.length; ++i) {
            var applet = plasmoid.applets[i]
            appletsForId[applet.id] = applet
        }

        //add applets present in AppletsOrder
        for (var i = 0; i < appletIds.length; ++i) {
            var id = appletIds[i]
            var applet = appletsForId[id]
            if (applet) {
                root.addApplet(applet, Qt.point(-1,-1));
                //discard it, so will be easy to find out who wasn't in the series
                appletsForId[id] = null
            }
        }

        for (var id in appletsForId) {
            var applet = appletsForId[id]
            if (applet) {
                root.addApplet(applet, Qt.point(-1,-1));
            }
        }

        saveOrder()
}
