/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "quickfontpreviewbackend.h"

#include <QFile>

#include <KLocalizedString>

#include <fontconfig/fontconfig.h>
#include <freetype/freetype.h>

#include "UnicodeBlocks.h"
#include "UnicodeScripts.h"

FontPreviewBackend::FontPreviewBackend(QObject *parent)
    : QObject(parent)
{
    // Initialize FreeType
    m_ftError = FT_Init_FreeType(&m_ftLibrary);
}

FontPreviewBackend::~FontPreviewBackend()
{
    if (m_ftLibrary) {
        FT_Done_FreeType(m_ftLibrary);
    }
}

void FontPreviewBackend::classBegin()
{
}

void FontPreviewBackend::componentComplete()
{
    m_ready = m_ftError == 0;
    QMetaObject::invokeMethod(this, &FontPreviewBackend::updateBlockRange);
}

int FontPreviewBackend::fontFace() const
{
    return m_fontFace;
}

void FontPreviewBackend::setFontFace(int id)
{
    if (m_fontFace == id) {
        return;
    }

    m_fontFace = id;
    Q_EMIT faceChanged();

    QMetaObject::invokeMethod(this, &FontPreviewBackend::updateBlockRange);
}

unsigned FontPreviewBackend::previewCount() const
{
    return m_previewCount;
}

void FontPreviewBackend::setPreviewCount(unsigned value)
{
    if (m_previewCount == value) {
        return;
    }

    m_previewCount = value;
    Q_EMIT previewCountChanged();

    QMetaObject::invokeMethod(this, &FontPreviewBackend::updateBlockRange);
}

QString FontPreviewBackend::fontName() const
{
    return m_fontName;
}

void FontPreviewBackend::setFontName(const QString &name)
{
    if (m_fontName == name) {
        return;
    }

    m_fontName = name;
    Q_EMIT nameChanged();

    QMetaObject::invokeMethod(this, &FontPreviewBackend::updateBlockRange);
}

FontPreviewBackend::Mode FontPreviewBackend::mode() const
{
    return m_mode;
}

void FontPreviewBackend::setMode(FontPreviewBackend::Mode newMode)
{
    if (m_mode == newMode) {
        return;
    }

    m_mode = newMode;

    Q_EMIT modeChanged();

    QMetaObject::invokeMethod(this, &FontPreviewBackend::updateBlockRange);
}

QStringList FontPreviewBackend::unicodeRangeNames() const
{
    return m_unicodeRangeNames;
}

int FontPreviewBackend::unicodeRangeIndex() const
{
    return m_unicodeRangeIndex;
}

void FontPreviewBackend::setUnicodeRangeIndex(int rangeId)
{
    if (m_unicodeRangeIndex == rangeId || rangeId < 0) {
        return;
    }

    m_unicodeRangeIndex = rangeId;
    Q_EMIT unicodeRangeChanged();

    QMetaObject::invokeMethod(this, &FontPreviewBackend::updateBlockRange);
}

QString FontPreviewBackend::characters() const
{
    return m_characters;
}

void FontPreviewBackend::updateBlockRange()
{
    if (!m_ready) {
        return;
    }

    m_unicodeRangeNames = QStringList{i18n("Standard Preview"), i18n("All Characters")};

    switch (m_mode) {
    case BlocksAndScripts:
        for (int i = 0; !constUnicodeBlocks[i].blockName.isEmpty(); ++i) {
            m_unicodeRangeNames.append(i18n("Unicode Block: %1", constUnicodeBlocks[i].blockName.toString()));
        }

        for (int i = 0; !constUnicodeScriptList[i].isEmpty(); ++i) {
            m_unicodeRangeNames.append(i18n("Unicode Script: %1", constUnicodeScriptList[i].toString()));
        }
        break;
    case ScriptsOnly:
        for (int i = 0; !constUnicodeScriptList[i].isEmpty(); ++i) {
            m_unicodeRangeNames.append(constUnicodeScriptList[i].toString());
        }
    case Basic:
    default:
        break;
    }

    Q_EMIT unicodeRangeNamesChanged();

    if (m_ftFace) {
        FT_Done_Face(m_ftFace);
    }

    // FT_New_Face requires absolute font path
    QByteArray fontPath;
    if (QFile::exists(m_fontName)) {
        fontPath = QFile::encodeName(m_fontName);
    } else {
        FcConfig *config = FcInitLoadConfigAndFonts();
        FcPattern *pat = FcNameParse((const FcChar8 *)(m_fontName.toUtf8().constData()));
        // Find the font
        FcResult res;
        FcPattern *font = FcFontMatch(config, pat, &res);
        if (font) {
            FcChar8 *file = NULL;
            if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
                // save the file to another std::string
                fontPath = (char *)file;
            }
            FcPatternDestroy(font);
        }
        FcPatternDestroy(pat);
    }

    m_ftError = FT_New_Face(m_ftLibrary, fontPath.constData(), m_fontFace, &m_ftFace);
    if (m_ftError) {
        return;
    }

    m_characters.clear();

    switch (m_unicodeRangeIndex) {
    case 0: // Standard Preview
        return;

    case 1: {
        FT_UInt index;
        unsigned charcode = FT_Get_First_Char(m_ftFace, &index);

        if (!index) {
            break;
        }

        m_characters += QChar(charcode);

        do {
            charcode = FT_Get_Next_Char(m_ftFace, charcode, &index);
            m_characters += QChar(charcode);
            // if FT_Get_Next_Char writes 0 to index then no more characters in font face
        } while (index && unsigned(m_characters.size()) < m_previewCount);

        break;
    }

    default: {
        Q_ASSERT(m_unicodeRangeIndex >= 2); // Should be checked by setUnicodeRange
        if (m_unicodeRangeIndex < m_unicodeRangeNames.size()) {
            const auto &unicodeBlock = constUnicodeBlocks[m_unicodeRangeIndex - 2];
            for (unsigned charcode = unicodeBlock.start; charcode < unicodeBlock.end; ++charcode) {
                m_characters += QChar(charcode);
            }
        } else {
            int scriptIndex = m_unicodeRangeIndex - m_unicodeRangeNames.size();

            for (int i = 0; constUnicodeScripts[i].scriptIndex >= 0; ++i) {
                if (constUnicodeScripts[i].scriptIndex != scriptIndex) {
                    continue;
                }
                const auto &unicodeBlock = constUnicodeScripts[i];
                for (unsigned charcode = unicodeBlock.start; charcode < unicodeBlock.end; ++charcode) {
                    m_characters += QChar(charcode);
                }
            }
        }
    }
    }

    Q_EMIT charactersChanged();
}
