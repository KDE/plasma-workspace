/*
    SPDX-FileCopyrightText: 2009 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "FontsPackage.h"
#include "KfiConstants.h"
#include "Misc.h"
#include <KZip>
#include <QDir>
#include <QTemporaryDir>

namespace KFI
{
namespace FontsPackage
{
QSet<QUrl> extract(const QString &fileName, QTemporaryDir **tempDir)
{
    QSet<QUrl> urls;

    if (!tempDir) {
        return urls;
    }

    KZip zip(fileName);

    if (zip.open(QIODevice::ReadOnly)) {
        const KArchiveDirectory *zipDir = zip.directory();

        if (zipDir) {
            QStringList fonts(zipDir->entries());

            if (!fonts.isEmpty()) {
                QStringList::ConstIterator it(fonts.begin()), end(fonts.end());

                for (; it != end; ++it) {
                    const KArchiveEntry *entry = zipDir->entry(*it);

                    if (entry && entry->isFile()) {
                        if (!(*tempDir)) {
                            (*tempDir) = new QTemporaryDir(QDir::tempPath() + "/" KFI_TMP_DIR_PREFIX);
                            (*tempDir)->setAutoRemove(true);
                        }

                        ((KArchiveFile *)entry)->copyTo((*tempDir)->path());

                        QString name(entry->name());

                        //
                        // Cant install hidden fonts, therefore need to
                        // unhide 1st!
                        if (Misc::isHidden(name)) {
                            ::rename(QFile::encodeName((*tempDir)->filePath(name)).data(), QFile::encodeName((*tempDir)->filePath(name.mid(1))).data());
                            name.remove(0, 1);
                        }

                        urls.insert(QUrl((*tempDir)->filePath(name)));
                    }
                }
            }
        }
    }

    return urls;
}

}

}
