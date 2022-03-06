/*
    SPDX-FileCopyrightText: 2018 Julian Wolff <wolff@julianwolff.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "previewrenderengine.h"
#include "Fc.h"

#include <QApplication>
#include <QScreen>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

#include <X11/Xft/Xft.h>

#ifdef HAVE_FONTCONFIG

static int qtToFcWeight(int weight)
{
    switch (weight) {
    case 0:
        return FC_WEIGHT_THIN;
    case QFont::Light >> 1:
        return FC_WEIGHT_EXTRALIGHT;
    case QFont::Light:
        return FC_WEIGHT_LIGHT;
    default:
    case QFont::Normal:
        return FC_WEIGHT_REGULAR;
    case (QFont::Normal + QFont::DemiBold) >> 1:
#ifdef KFI_HAVE_MEDIUM_WEIGHT
        return FC_WEIGHT_MEDIUM;
#endif
    case QFont::DemiBold:
        return FC_WEIGHT_DEMIBOLD;
    case QFont::Bold:
        return FC_WEIGHT_BOLD;
    case (QFont::Bold + QFont::Black) >> 1:
        return FC_WEIGHT_EXTRABOLD;
    case QFont::Black:
        return FC_WEIGHT_BLACK;
    }
}

#ifndef KFI_FC_NO_WIDTHS
static int qtToFcWidth(int weight)
{
    switch (weight) {
    case QFont::UltraCondensed:
        return KFI_FC_WIDTH_ULTRACONDENSED;
    case QFont::ExtraCondensed:
        return KFI_FC_WIDTH_EXTRACONDENSED;
    case QFont::Condensed:
        return KFI_FC_WIDTH_CONDENSED;
    case QFont::SemiCondensed:
        return KFI_FC_WIDTH_SEMICONDENSED;
    default:
    case QFont::Unstretched:
        return KFI_FC_WIDTH_NORMAL;
    case QFont::SemiExpanded:
        return KFI_FC_WIDTH_SEMIEXPANDED;
    case QFont::Expanded:
        return KFI_FC_WIDTH_EXPANDED;
    case QFont::ExtraExpanded:
        return KFI_FC_WIDTH_EXTRAEXPANDED;
    case QFont::UltraExpanded:
        return KFI_FC_WIDTH_ULTRAEXPANDED;
    }
}
#endif

static bool qtToFcSlant(int slant)
{
    switch (slant) {
    default:
    case QFont::StyleNormal:
        return FC_SLANT_ROMAN;
    case QFont::StyleItalic:
        return FC_SLANT_ITALIC;
    case QFont::StyleOblique:
        return FC_SLANT_OBLIQUE;
    }
}

static quint32 qtToFcStyle(const QFont &font)
{
    return KFI::FC::createStyleVal(qtToFcWeight(font.weight()), qtToFcWidth(font.stretch()), qtToFcSlant(font.style()));
}

PreviewRenderEngine::PreviewRenderEngine(bool init)
    : CFcEngine(init)
{
    if (init)
        FcInitReinitialize();
}

PreviewRenderEngine::~PreviewRenderEngine()
{
}

QImage PreviewRenderEngine::drawAutoSize(const QFont &font, const QColor &txt, const QColor &bgnd, const QString &text)
{
    const QString &name = font.family();
    const quint32 style = qtToFcStyle(font);
    int faceNo = 0;

    double ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
    double dpi = QX11Info::appDpiY();

    int fSize((int)(((font.pointSizeF() * dpi * ratio) / 72.0) + 0.5));

    QImage image(draw(name, style, faceNo, txt, bgnd, fSize, text));
    image.setDevicePixelRatio(ratio);
    return image;
}

#endif // HAVE_FONTCONFIG
