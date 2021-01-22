/*
 * Copyright (C) 2020 Carson Black <uhhadd@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QQC2
import org.kde.kirigami 2.14 as Kirigami
import org.kde.userfeedback 1.0 as UserFeedback
import org.kde.kcm 1.3

ScrollViewKCM {
    title: i18n("Data Collection History")

    UserFeedback.AuditLogUiController {
        id: auditlogController
    }

    Kirigami.PlaceholderMessage {
        text: i18n("We haven't collected any data from you yet. Check back later.")
        anchors.centerIn: parent
        visible: !auditlogController.hasLogEntries
    }

    ListView {
        model: auditlogController.logEntryModel

        delegate: Kirigami.BasicListItem {
            label: model.display
            subtitle: model.data
        }
    }
}