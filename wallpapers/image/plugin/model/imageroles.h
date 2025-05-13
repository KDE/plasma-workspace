/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

class ImageRoles
{
    Q_GADGET

public:
    enum RoleType {
        AuthorRole = Qt::UserRole,
        PreviewRole,
        PathRole,
        PackageNameRole,
        RemovableRole,
        PendingDeletionRole,
        ToggleRole, /**< Used in slideshow model */
        SelectorsRole,
    };
    Q_ENUM(RoleType)
};
