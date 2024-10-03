/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.notificationmanager as NotificationManager

Loader {
    id: footerLoader
    Layout.fillWidth: true

    property ModelInterface modelInterface

    visible: active
    sourceComponent: {
        if (modelInterface.notificationType === NotificationManager.Notifications.JobType) {
            return jobComponent;
        } else if (modelInterface.urls.length > 0) {
            return thumbnailComponent
        } else if (modelInterface.actionNames.length > 0 || modelInterface.hasReplyAction) {
            return actionComponent;
        }
        return undefined;
    }

    // Actions
    Component {
        id: actionComponent
        ActionContainer {
            id: actionContainer
            Layout.fillWidth: true
            modelInterface: footerLoader.modelInterface
        }
    }

    // Jobs
    Component {
        id: jobComponent
        JobItem {
            modelInterface: footerLoader.modelInterface
            iconContainerItem: iconContainer //FIXME
        }
    }

    // Thumbnails (contains actions as well)
    Component {
        id: thumbnailComponent
        ThumbnailStrip {
            modelInterface: footerLoader.modelInterface
        }
    }
}
