#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Family.h"
#include <QString>

namespace KFI
{
namespace Utils
{
enum EFileType {
    FILE_INVALID,
    FILE_BITMAP,
    FILE_SCALABLE,
    FILE_AFM,
    FILE_PFM,
};

extern bool isAAfm(const QString &fname);
extern bool isAPfm(const QString &fname);
extern bool isAType1(const QString &fname);
extern void createAfm(const QString &file, EFileType type);
extern EFileType check(const QString &file, Family &fam);

}

}
