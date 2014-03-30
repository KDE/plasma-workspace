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
    return "http://imgur.com/api/upload";
}

function contentKey() {
    return "image";
}

function setup() {
    // key associated with plasma-devel@kde.org
    // thanks to Alan Schaaf of Imgur (alan@imgur.com)
    provider.addPostItem("key", "d0757bc2e94a0d4652f28079a0be9379", "text/plain");
}

function handleResultData(data) {
    var res = provider.parseXML("original_image", data);
    if (res == "") {
        provider.error(data);
        return;
    }
    provider.success(res);
}
