#pragma once

/*
 * SPDX-FileCopyrightText: 2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

class QTemporaryDir;

#include <QSet>
#include <QUrl>

namespace KFI
{
namespace FontsPackage
{
QSet<QUrl> extract(const QString &fileName, QTemporaryDir **tempDir);
}

}
