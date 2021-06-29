/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "FontThumbnail.h"
#include "KfiConstants.h"
#include <KZip>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QMimeDatabase>
#include <QPalette>
#include <QTemporaryDir>

#include "debug.h"

extern "C" {
Q_DECL_EXPORT ThumbCreator *new_creator()
{
    return new KFI::CFontThumbnail;
}
}

namespace KFI
{
CFontThumbnail::CFontThumbnail()
{
}

bool CFontThumbnail::create(const QString &path, int width, int height, QImage &img)
{
    QString realPath(path);
    QTemporaryDir *tempDir = nullptr;

    qCDebug(KCM_KFONTINST_THUMBNAIL) << "Create font thumbnail for:" << path << Qt::endl;

    // Is this a appliaction/vnd.kde.fontspackage file? If so, extract 1 scalable font...
    QMimeDatabase db;
    if (Misc::isPackage(path) || "application/zip" == db.mimeTypeForFile(path, QMimeDatabase::MatchContent).name()) {
        KZip zip(path);

        if (zip.open(QIODevice::ReadOnly)) {
            const KArchiveDirectory *zipDir = zip.directory();

            if (zipDir) {
                QStringList fonts(zipDir->entries());

                if (!fonts.isEmpty()) {
                    QStringList::ConstIterator it(fonts.begin()), end(fonts.end());

                    for (; it != end; ++it) {
                        const KArchiveEntry *entry = zipDir->entry(*it);

                        if (entry && entry->isFile()) {
                            delete tempDir;
                            tempDir = new QTemporaryDir(QDir::tempPath() + "/" KFI_TMP_DIR_PREFIX);
                            tempDir->setAutoRemove(true);

                            ((KArchiveFile *)entry)->copyTo(tempDir->path());

                            QString mime(db.mimeTypeForFile(tempDir->filePath(entry->name())).name());

                            if (mime == "font/ttf" || mime == "font/otf" || mime == "application/x-font-ttf" || mime == "application/x-font-otf"
                                || mime == "application/x-font-type1") {
                                realPath = tempDir->filePath(entry->name());
                                break;
                            } else {
                                ::unlink(QFile::encodeName(tempDir->filePath(entry->name())).data());
                            }
                        }
                    }
                }
            }
        }
    }

    QColor bgnd(Qt::black);

    bgnd.setAlpha(0);
    img = itsEngine.draw(realPath, KFI_NO_STYLE_INFO, 0, QApplication::palette().text().color(), bgnd, width, height, true);

    delete tempDir;
    return !img.isNull();
}

ThumbCreator::Flags CFontThumbnail::flags() const
{
    return None;
}

}
