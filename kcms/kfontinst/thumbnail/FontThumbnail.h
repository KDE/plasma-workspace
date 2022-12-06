#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KIO/ThumbnailCreator>

#include "FcEngine.h"

class CFontThumbnail : public KIO::ThumbnailCreator
{
public:
    CFontThumbnail(QObject *parent, const QVariantList &args);
    ~CFontThumbnail() override;

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;

private:
    KFI::CFcEngine m_engine;
};
