/*
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15

import org.kde.kirigami 2.20 as Kirigami

/*
 * A model with a list of users to show in the view.
 * There are different implementations in sddm greeter (UserModel) and
 * KScreenLocker (SessionsModel), so some roles will be missing.
 *
 * type: {
 *  name: string,
 *  realName: string,
 *  homeDir: string,
 *  icon: string,
 *  iconName?: string,
 *  needsPassword?: bool,
 *  displayNumber?: string,
 *  vtNumber?: int,
 *  session?: string
 *  isTty?: bool,
 * }
 */
ListView {
    id: view
    readonly property bool constrainText: count > 1
    readonly property string selectedUser: currentItem?.userName ?? ""
    readonly property bool selectedUserNeedsPassword: currentItem?.needsPassword ?? false
    readonly property size delegateSize: {
        let size = Qt.size(0, 0)
        if (count === 1 && currentItem !== null) {
            size.width = currentItem.implicitWidth
            size.height = currentItem.implicitHeight
        } else {
            let component = Qt.createComponent("UserDelegate.qml")
            let item = component.createObject(null, {
                "highlighted": true,
                "constrainText": view.constrainText,
                "text": "has\nthree\nlines"
            })
            size.width = item.implicitWidth
            size.height = item.implicitHeight
        }
        return size
    }

    // ImplicitWidth is wide enough to contain all items
    // even when the first or last item is the current item.
    implicitWidth: Math.max(delegateSize.width, delegateSize.width * count * 2 - delegateSize.width)
    implicitHeight: delegateSize.height
    baselineOffset: height - bottomMargin

    activeFocusOnTab: true

    /*
     * Signals that a user was explicitly selected
     */
    signal userSelected()

    orientation: ListView.Horizontal
    highlightRangeMode: ListView.StrictlyEnforceRange

    //centre align selected item (which implicitly centre aligns the rest
    // preferredHighlightBegin should be the left side of the center delegate
    // preferredHighlightEnd should be the right side of the center delegate
    preferredHighlightBegin: (width - delegateSize.width) / 2
    preferredHighlightEnd: preferredHighlightBegin + delegateSize.width
    highlightMoveDuration: Kirigami.Units.veryLongDuration
    highlightResizeDuration: Kirigami.Units.veryLongDuration

    // Disable flicking if we only have on user (like on the lockscreen)
    interactive: count > 1

    Accessible.role: Accessible.List

    delegate: UserDelegate {
        avatarPath: model.icon ?? ""
        iconName: model.iconName ?? ""
        needsPassword: model.needsPassword ?? true

        displayName: {
            const displayName = model.realName || model.name

            if (model.vtNumber === undefined || model.vtNumber < 0) {
                return displayName
            }

            if (!model.session) {
                return i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Nobody logged in on that session", "Unused")
            }


            let location = undefined
            if (model.isTty) {
                location = i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "User logged in on console number", "TTY %1", model.vtNumber)
            } else if (model.displayNumber) {
                location = i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "User logged in on console (X display number)", "on TTY %1 (Display %2)", model.vtNumber, model.displayNumber)
            }

            if (location !== undefined) {
                return i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Username (location)", "%1 (%2)", displayName, location)
            }

            return displayName
        }

        userName: model.name

        height: view.contentItem.height

        //if we only have one delegate, we don't need to clip the text as it won't be overlapping with anything
        constrainText: view.constrainText

        highlighted: ListView.isCurrentItem

        onClicked: {
            ListView.view.currentIndex = index;
            ListView.view.userSelected();
        }
    }

    Keys.onEscapePressed: view.userSelected()
    Keys.onEnterPressed: view.userSelected()
    Keys.onReturnPressed: view.userSelected()
}
