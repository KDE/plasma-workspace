/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <optional>

#include <QSqlDatabase>

namespace DatabaseUtils
{
enum Direction {
    MemToFile,
    FileToMem,
};

enum Location {
    LocalFile,
    Memory,
};

QString databaseFolder();
QString databaseName(Location location);
QString databaseConnectionName(Location location);
std::optional<QSqlDatabase> openDatabase(Location location);
std::optional<QSqlDatabase> migrate(Direction direction);
};
