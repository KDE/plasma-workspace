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
    return "http://paste.kde.org/index.php";
}

function method() {
    return "POST";
}

function contentKey() {
    return "paste_data";
}

function setup() {
    provider.addQueryItem("api_submit", "true");
    provider.addQueryItem("mode", "xml");
    provider.addQueryItem("paste_lang", "text");
}

function handleResultData(data) {
    var res = provider.parseXML("id", data);
    if (res == "") {
        provider.error(data);
        return;
    }
    provider.success("http://paste.kde.org/" + res);
}

