/*
   Copyright (C) 2020 David Edmundson <davidedmundson@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "waylandclipboard.h"

#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QPointer>

#include <qpa/qplatformnativeinterface.h>

WaylandClipboard::WaylandClipboard(QObject *parent)
    : SystemClipboard(parent)
{
    connect(&m_dataControl, &DataControl::receivedSelectionChanged, this, [this] {
        emit changed(QClipboard::Clipboard);
    });
    connect(&m_dataControl, &DataControl::selectionChanged, this, [this] {
        emit changed(QClipboard::Clipboard);
    });
}

void WaylandClipboard::setMimeData(QMimeData *mime, QClipboard::Mode mode)
{
    if (mode == QClipboard::Clipboard) {
        m_dataControl.setSelection(mime);
    }
}

void WaylandClipboard::clear(QClipboard::Mode mode)
{
    if (mode == QClipboard::Clipboard) {
        m_dataControl.clearSelection();
    } else if (mode == QClipboard::Selection) {
        m_dataControl.clearPrimarySelection();
    }
}

const QMimeData *WaylandClipboard::mimeData(QClipboard::Mode mode) const
{
    if (mode == QClipboard::Clipboard) {
        // return our locally set selection if it's not cancelled to avoid copying data to ourselves
        return m_dataControl.selection() ? m_dataControl.selection() : m_dataControl.receivedSelection();
    }
    return nullptr;
}

