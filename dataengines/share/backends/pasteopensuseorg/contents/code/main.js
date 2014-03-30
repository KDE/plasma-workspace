/***************************************************************************
 *   Copyright (C) 2010 by Will Stephenson <wstephenson@kde.org>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

function url() {
    return "http://paste.opensuse.org";
}

function contentKey() {
    return "code";
}

function setup() {
    provider.addQueryItem("name", "KDE");
    provider.addQueryItem("title", "mypaste");
    provider.addQueryItem("lang", "text");
    provider.addQueryItem("expire", "1440");
    provider.addQueryItem("submit", "submit");
}

function handleResultData(data) {
    var res = data.match("(Info.+)");
    if (res != "") {
        return;
    }
    provider.error(data);
}

function handleRedirection(url) {
    provider.success(url);
}
