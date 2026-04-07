/*
    SPDX-FileCopyrightText: Ⓒ 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "notificationmanager_export.h"

#include <QStringList>

class NotifyRcPaths
{
public:
    /** All search paths for notifyrc files.
     *  That's those specified by XDG_DATA_DIRS as well as those
     *  of installed Flatpak applications.
     */
    NOTIFICATIONMANAGER_EXPORT static QStringList allSearchPaths();

    /** Locate the first notifyrc file in the search paths with the given name. */
    static QString locate(QStringView name);
};
