/*
 *   Copyright 2012 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2012 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
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

var weekdays = ["Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"]
var months = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"]

function daysInMonth(month)
{
    if (month==2) {
        if (isLeap(year)) return 29;
        else return 28;
    }
    else if (month<8) {
        if (month%2==1) return 31;
        else return 30;
    }
    else {
        if (month%2==0) return 31;
        else return 30;
    }
}

function isLeap(year)
{
    return ((year%100==0 && year%400==0) || (year%4==0 && year%100!=0));
}

function getYear(date)
{
    return parseInt(Qt.formatDate(date, "yyyy"));
}

function getMonth(date)
{
    return parseInt(Qt.formatDate(date, "M"));
}

function getDate(date)
{
    return parseInt(Qt.formatDate(date, "d"));
}

function getWeekday(date)
{
}
