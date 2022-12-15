/*
    SPDX-FileCopyrightText: 2022 Tanbir Jishan <tantalising007@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.workspace.calendar 2.0


ListView {
    id: infiniteList

    readonly property double cellHeight: currentItem ? currentItem.cellHeight : 0
    readonly property double cellWidth: currentItem ? currentItem.cellWidth : 0

    required property var backend
    required property int viewType

    enum ViewType {
        DayView,
        YearView,
        DecadeView
    }

    property bool changeDate: false // To control whether to animate or animate + date change after start up complete. Should always be false on start up.
    property bool dragHandled: false

    highlightRangeMode: ListView.StrictlyEnforceRange
    snapMode: ListView.SnapToItem
    highlightMoveDuration: PlasmaCore.Units.longDuration
    highlightMoveVelocity: -1
    reuseItems: true
    model: 3
    keyNavigationEnabled: false // It's actually enabled. The default behaviour is not desirable

    function focusFirstCellOfView() {
        currentItem.repeater.itemAt(0).forceActiveFocus(Qt.TabFocusReason);
    }

    function resetViewPosition() {
        positionViewAtIndex(1, ListView.Beginning);
        currentIndex = 1;
    }

    // prevents keep changing the date by dragging and holding; returns true if date should not change else returns false
    function handleDrag() {
        if (dragHandled) { // if already date changed for dragging once (drag still going on like streams) then
            resetViewPosition(); //reset so that further dragging is not broken when this dragging session is over
            return true;        // return true so that still ongoing drags do not change date
        } else { //  else if drag not handled then
            if (draggingVertically) { // if reached view edge because of drag (and not wheel or buttons) then
                dragHandled = true; // mark as drag handled so that next drags in stream are ignored
            }
        }
        return false; // return false because we need to change date either for first drag or for view changed by some means other than drag
    }

    // These signal handlers animate the view. They are the only ones through which date (and should as well) changes.
    onAtYEndChanged: {
        if (atYEnd) {
            if (handleDrag()) return;
            changeDate ? nextView() : changeDate = true;
            resetViewPosition();
        }
    }

    onAtYBeginningChanged: {
        if (atYBeginning) {
            if (handleDrag()) return;
            changeDate ? previousView() : changeDate = true;
            resetViewPosition();
        }
    }

    onDraggingVerticallyChanged: if (draggingVertically === false) dragHandled = false; //reset the value when drag ends

    Component.onCompleted: {
        // set up alternative model for delegates at edges
        // so that they can be set up to always show the right date (top: previous date; bottom: next date)
        // date here means what the respective views should show(e.g. MonthView date -> month)
        // this prevents them from showing the current date when they are being animated out of/into view
        // since different years don't have different names for months, we don't need to set up alternative models for YearView

        switch(infiniteList.viewType) {
            case InfiniteList.ViewType.DayView:
                infiniteList.itemAtIndex(0).gridModel = previousAlternativeBackend.item.daysModel;
                infiniteList.itemAtIndex(2).gridModel = nextAlternativeBackend.item.daysModel;
                break;

            case InfiniteList.ViewType.DecadeView:
                infiniteList.itemAtIndex(0).gridModel = previousYearModel.item;
                infiniteList.itemAtIndex(2).gridModel = nextYearModel.item;
                break;
        }
    }

    /* ------------------------------- UI ENDS ----------------- MODEL MANIPULATING FUNCTIONS ----------------------------------- */


    // used to update the alternative decadeview models when year changes
    function updateDecadeOverview(offset) {
        if (Math.abs(offset) !== 1) return;

        const model = offset == 1 ? nextYearModel.item: previousYearModel.item;
        const year = backend.year + (10 * offset) // Increase or decrease year by a decade
        const decade = year - year % 10;

        for (let i = 0, j = model.count; i < j; ++i) { // aware
            const label = decade - 1 + i;

            model.setProperty(i, "yearNumber", label);
            model.setProperty(i, "label", label);
        }
    }

    function initYearModel(offset) {
        if (Math.abs(offset) !== 1) return;

        const model = offset == 1 ? nextYearModel.item: previousYearModel.item;
        for (let i = 0; i < 12; ++i) {
            model.append({
                label: 2050, // this value will be overwritten, but it set the type of the property to int
                yearNumber: 2050,
                isCurrent: (i > 0 && i < 11) // first and last year are outside the decade
            })
        }

        infiniteList.updateDecadeOverview(offset);
    }

    function modulo(a:int, n: int) { // always keep the 'a' between [1, n]
        return ((((a - 1) % n) + n) % n) + 1;
    }

    function previousView() {
        switch(infiniteList.viewType) {
            case InfiniteList.ViewType.DayView:
                backend.previousMonth();
                break;

            case InfiniteList.ViewType.YearView:
                backend.previousYear();
                break;

            case InfiniteList.ViewType.DecadeView:
                backend.previousDecade();
                break;
        }
    }

    function nextView() {
        switch(infiniteList.viewType) {
            case InfiniteList.ViewType.DayView:
                backend.nextMonth();
                break;

            case InfiniteList.ViewType.YearView:
                backend.nextYear();
                break;

            case InfiniteList.ViewType.DecadeView:
                backend.nextDecade();
                break;
        }
    }

    /*----------------------------------------------------- alternative models ---------------------------------------------------------------*/

    Loader {
        id: previousAlternativeBackend

        active: infiniteList.viewType === InfiniteList.ViewType.DayView
        asynchronous: true

        sourceComponent: Calendar {
            days: backend.days
            weeks: backend.weeks
            firstDayOfWeek: backend.firstDayOfWeek
            today: backend.today

            function goToPreviousView() {
               const month = modulo(backend.month - 1, 12)
               const year = month === 12 ? backend.year - 1 : backend.year
               goToYear(year);
               goToMonth(month);
           }
       }
       onStatusChanged: if (status === Loader.Ready) item.goToPreviousView();
    }

    Loader {
        id: nextAlternativeBackend

        active: infiniteList.viewType === InfiniteList.ViewType.DayView
        asynchronous: true

        sourceComponent: Calendar {
            days: backend.days
            weeks: backend.weeks
            firstDayOfWeek: backend.firstDayOfWeek
            today: backend.today

            function goToNextView() {
                const month = modulo(backend.month + 1, 12)
                const year = month === 1 ? backend.year + 1 : backend.year
                goToYear(year);
                goToMonth(month);
            }
        }
        onStatusChanged: if (status === Loader.Ready) item.goToNextView(); //clear other model names
    }

    Loader {
        id: nextYearModel

        active: infiniteList.viewType === InfiniteList.ViewType.DecadeView
        asynchronous: true

        function update() { infiniteList.updateDecadeOverview(1) }

        sourceComponent: ListModel {}
        onStatusChanged: if (nextYearModel.status === Loader.Ready) infiniteList.initYearModel(1);
    }

    Loader {
        id: previousYearModel

        active: infiniteList.viewType === InfiniteList.ViewType.DecadeView
        asynchronous: true

        function update() { infiniteList.updateDecadeOverview(-1) }

        sourceComponent: ListModel {}
        onStatusChanged: if (previousYearModel.status === Loader.Ready) infiniteList.initYearModel(-1);
    }

    Connections {
        target: backend
        enabled: infiniteList.viewType === InfiniteList.ViewType.DayView

        function onMonthChanged() {
            previousAlternativeBackend.item.goToPreviousView();
            nextAlternativeBackend.item.goToNextView();
        }
    }

    Connections {
        target: backend
        enabled: infiniteList.viewType === InfiniteList.ViewType.DecadeView

        function onYearChanged() {
            nextYearModel.update();
            previousYearModel.update();
        }
    }
}
