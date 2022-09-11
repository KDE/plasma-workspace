/*
    SPDX-FileCopyrightText: 2022 Tanbir Jishan <tantalising007@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.workspace.calendar 2.0

Item {
    id: root

    required property var backend
    required property int viewType
    property QtObject eventPluginsManager
    property alias delegate: infiniteRepeater.delegate
    readonly property alias currentItem: infiniteList.currentItem

    enum ViewType {
        DayView,
        YearView,
        DecadeView
    }

    enum AnimationDirection {
        Upward,
        Downward
    }

    SwipeView {
        id: infiniteList

        anchors.fill: parent
        orientation: Qt.Vertical
        currentIndex: 1 //middle of the view, currentIndex always returns back to middle so that user can flick both upward and downward

        property bool handlingIndexChange: false // This var is used to prevent date change ⇆ index change loop
        property var highlightMoveDuration: PlasmaCore.Units.longDuration
        property int lastMonth: -1
        property int lastYear: -1

        Connections {
            id: dateToViewSynchroniser
            target: root.backend

            // Animation is done by moving to the edge with zero animation
            // duration and then coming with a non-zero animation duration
            onMonthChanged: infiniteList.animateDateChange()
            onYearChanged: infiniteList.animateDateChange()
        }

        Repeater {
            id: infiniteRepeater
            model: 3
        }

        onCurrentIndexChanged: adjustDate()

        Component.onCompleted: {
            //init vars. last* vars are for tracking whether date changed toward future or past
            contentItem.highlightMoveDuration = highlightMoveDuration;
            lastMonth = root.backend.month - 1;
            lastYear = root.backend.year;

            // set up alternative model for delegates at edges
            // so that they can be set up to always shows the last state of the main model
            // date here means what the respective views should show(e.g. MonthView date -> month)
            // this prevents them from showing the current date when they are being animated out of view
            // since different years don't have different names for months, we don't need to set up alternative models for YearView
            var alternativeModel = undefined;
            if (root.viewType === InfiniteList.ViewType.DayView) {
                alternativeModel = backend.daysModel;
            } else if (root.viewType === InfiniteList.ViewType.DecadeView) {
                alternativeModel = yearModel;
            }
            if (alternativeModel !== undefined) {
                infiniteRepeater.itemAt(0).gridModel = alternativeModel;
                infiniteRepeater.itemAt(2).gridModel = alternativeModel;
            }
        }

/*----------------------------------------------------- helper functions ---------------------------------------------------------------*/

        function resetIndexTo(index: int, duration = 0) {
            contentItem.highlightMoveDuration = duration;
            if (currentIndex !== index) {
                currentIndex = index;
            }
        }

        function changeDateOfView() {
            const swipedUp = currentIndex == 2;

            switch(root.viewType) {
                case InfiniteList.ViewType.DayView:
                    swipedUp ? root.backend.nextMonth() : root.backend.previousMonth();
                    break;

                case InfiniteList.ViewType.YearView:
                    swipedUp ? root.backend.nextYear() : root.backend.previousYear();
                    break;

                case InfiniteList.ViewType.DecadeView:
                    swipedUp ? root.backend.nextDecade() : root.backend.previousDecade();
                    break;
            }
        }

        function adjustDate() {
            const inMiddle = currentIndex == 1;
            if (handlingIndexChange || inMiddle) return;

            handlingIndexChange = true;
            changeDateOfView();
            resetIndexTo(1); //back to middle
            handlingIndexChange = false;
        }

        function animate(direction) {
            if (handlingIndexChange) return;

            const targetIndex = (direction === InfiniteList.AnimationDirection.Upward) ? 0 : 2;

            handlingIndexChange = true;
            resetIndexTo(targetIndex); //move to edge from middle
            resetIndexTo(1, highlightMoveDuration); // come back to middle with non-zero animation duration
            handlingIndexChange = false;
        }

        function animateDateChange(toFuture = undefined) {
            const month = root.backend.month - 1;
            const year = root.backend.year;
            let goToFuture = false;

            if(toFuture === undefined) {
                switch(root.viewType) {
                    case InfiniteList.ViewType.DayView:
                        if (month === lastMonth) return;
                        goToFuture = (month > lastMonth || year > lastYear) && !(year < lastYear);
                        break;

                    default:
                        if (year === lastYear) return;
                        goToFuture = year > lastYear;
                        break;
                }
            } else {
                goToFuture = toFuture;
            }


            if (goToFuture) {
                animate(InfiniteList.AnimationDirection.Upward);
            } else {
                animate(InfiniteList.AnimationDirection.Downward);
            }

            lastMonth = month;
            lastYear = year;
        }

        // used to update the alternative decadeview models when year changes
        function updateDecadeOverview() {
            const date = backend.displayedDate;
            const day = date.getDate();
            const month = date.getMonth() + 1;
            const year = date.getFullYear();
            const decade = year - year % 10;

            for (let i = 0, j = yearModel.count; i < j; ++i) {
                const label = decade - 1 + i;
                yearModel.setProperty(i, "yearNumber", label);
                yearModel.setProperty(i, "label", label);
            }
        }
/*----------------------------------------------------- alternative models ---------------------------------------------------------------*/

        Calendar {
            id: backend

            days: root.backend.days
            weeks: root.backend.weeks
            firstDayOfWeek: root.backend.firstDayOfWeek
            today: root.backend.today

            Component.onCompleted: {
                daysModel.setPluginsManager(root.eventPluginsManager);
            }
        }

        ListModel {
            id: yearModel

            Component.onCompleted: {
                for (let i = 0; i < 12; ++i) {
                    append({
                        label: 2050, // this value will be overwritten, but it set the type of the property to int
                        yearNumber: 2050,
                        isCurrent: (i > 0 && i < 11) // first and last year are outside the decade
                    })
                }
                infiniteList.updateDecadeOverview();
            }
        }
    }

/*----------------------------------------------------- public functions ---------------------------------------------------------------*/

    function nextView() {
        switch(root.viewType) {
            case InfiniteList.ViewType.DayView:
                backend.goToMonth(root.backend.month);
                backend.goToYear(root.backend.year);
                root.backend.nextMonth();
                break;

            case InfiniteList.ViewType.YearView:
                root.backend.nextYear();
                break;

            case InfiniteList.ViewType.DecadeView:
                backend.goToYear(root.backend.year);
                infiniteList.updateDecadeOverview();
                root.backend.nextDecade();
                break;
        }
    }

    function previousView() {
        switch(root.viewType) {
            case InfiniteList.ViewType.DayView:
                backend.goToMonth(root.backend.month);
                backend.goToYear(root.backend.year);
                root.backend.previousMonth();
                break;

            case InfiniteList.ViewType.YearView:
                root.backend.previousYear();
                break;

            case InfiniteList.ViewType.DecadeView:
                backend.goToYear(root.backend.year);
                infiniteList.updateDecadeOverview();
                root.backend.previousDecade();
                break;
        }
    }

    function resetToToday() {
        backend.goToMonth(root.backend.month);
        backend.goToYear(root.backend.year);
        root.backend.resetToToday();
    }

    function focusFirstCellOfView() {
        infiniteList.currentItem.repeater.itemAt(0).forceActiveFocus(Qt.TabFocusReason);
        infiniteList.resetIndexTo(1)
        infiniteList.currentItem.repeater.itemAt(0).forceActiveFocus(Qt.TabFocusReason);
    }
}
