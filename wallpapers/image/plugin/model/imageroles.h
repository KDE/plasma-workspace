/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef IMAGEROLES_H
#define IMAGEROLES_H

class ImageRoles
{
    Q_GADGET

public:
    enum RoleType {
        AuthorRole = Qt::UserRole,
        ScreenshotRole,
        ResolutionRole,
        PathRole,
        PackageNameRole,
        RemovableRole,
        PendingDeletionRole,
        DynamicTypeRole, /**< Used in dynamic wallpaper package */
        ToggleRole, /**< Used in slideshow model */
    };
    Q_ENUM(RoleType)
};

#endif
