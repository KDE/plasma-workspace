/*
    SPDX-FileCopyrightText: 2018 Julian Wolff <wolff@julianwolff.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "FcEngine.h"
#include "kxftconfig.h"

#ifdef HAVE_FONTCONFIG

#include <QImage>

class PreviewRenderEngine : public KFI::CFcEngine
{
public:
    PreviewRenderEngine(bool init = true);
    ~PreviewRenderEngine() override;

    QImage drawAutoSize(const QFont &font, const QColor &txt, const QColor &bgnd, const QString &text);
};

#endif // HAVE_FONTCONFIG
