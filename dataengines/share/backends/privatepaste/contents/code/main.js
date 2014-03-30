/***************************************************************************
 *   Copyright (C) 2010 by Artur Duque de Souza <asouza@kde.org>           *
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
    return "http://privatepaste.com/save";
}

function contentKey() {
    return "paste_content";
}

function setup() {
    provider.addQueryItem("_xsrf", "d38415c699a0408ebfce524c2dc0a4ba");
    provider.addQueryItem("formatting", "No Formatting");
    provider.addQueryItem("line_numbers", "on");
    provider.addQueryItem("expire", "31536000");
    provider.addQueryItem("secure_paste", "off");
}

function handleResultData(data) {
    var res = data.match("(Error.+)");
    if (res != "") {
        return;
    }
    provider.error(data);
}

function handleRedirection(url) {
    provider.success(url);
}
