#ifndef __FONT_THUMBNAIL__H__
#define __FONT_THUMBNAIL__H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KIO/ThumbCreator>

#include "FcEngine.h"

namespace KFI
{
class CFontThumbnail : public ThumbCreator
{
public:
    CFontThumbnail();
    ~CFontThumbnail() override
    {
    }

    bool create(const QString &path, int width, int height, QImage &img) override;
    Flags flags() const override;

private:
    CFcEngine itsEngine;
};

}

#endif
