/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "quickfontpreviewitem.h"
#include "config-X11.h"

#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPainter>
#include <QPalette>

#include <KLocalizedString>

#include "Fc.h"
#include "UnicodeBlocks.h"
#include "UnicodeScripts.h"
#include <fontconfig/fontconfig.h>
#include <freetype/ftmodapi.h>

namespace KFI
{
QString fontDescriptiveName(const QString &fontPath, int face)
{
    int count = 0;
    FcPattern *pat = FcFreeTypeQuery((const FcChar8 *)(QFile::encodeName(fontPath).constData()), face, nullptr, &count);
    if (!pat) {
        return QString();
    }
    QString name = FC::createName(pat);
    FcPatternDestroy(pat);
    return name;
}

QImage glyphToImage(FT_GlyphSlot slot)
{
    int height = slot->bitmap.rows;
    int width = slot->bitmap.width;
    int pitch = (width + 3) & ~3;
    int glyph_buffer_size = height * pitch;
    std::unique_ptr<uchar[]> glyph_buffer(new uchar[glyph_buffer_size]);
    Q_ASSERT(slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
    uchar *src = slot->bitmap.buffer;
    uchar *dst = glyph_buffer.get();
    int h = slot->bitmap.rows;
    int bytes = width;
    while (h--) {
        memcpy(dst, src, bytes);
        dst += pitch;
        src += slot->bitmap.pitch;
    }

    return QImage(glyph_buffer.get(), width, height, pitch, QImage::Format_Alpha8);
}
}

FontPreviewItem::FontPreviewItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_styleInfo(KFI_NO_STYLE_INFO)
{
    setFlag(ItemHasContents, true);
    setOpaquePainting(true);

    setFillColor(qGuiApp->palette().base().color());
    connect(qGuiApp, &QGuiApplication::paletteChanged, this, [this](const QPalette &newPalette) {
        setFillColor(newPalette.base().color());
    });

    // Initialize FreeType
    m_ftError = FT_Init_FreeType(&m_ftLibrary);
    if (m_ftError) {
        return;
    }
    FT_Bool no_darkening = false;
    FT_Property_Set(m_ftLibrary, "cff", "no-stem-darkening", &no_darkening);
}

FontPreviewItem::~FontPreviewItem()
{
    if (m_ftLibrary) {
        FT_Done_FreeType(m_ftLibrary);
    }
}

void FontPreviewItem::componentComplete()
{
    QQuickItem::componentComplete();
    m_ready = m_ftError == 0;
    polish();
}

QString FontPreviewItem::fontName() const
{
    return m_fontName;
}

void FontPreviewItem::setFontName(const QString &newName)
{
    if (m_fontName == newName) {
        return;
    }

    m_fontName = newName;
    Q_EMIT nameChanged();

    m_unicodeRangeChanged = true;
    polish();
}

int FontPreviewItem::fontFace() const
{
    return m_currentFace;
}

void FontPreviewItem::setFontFace(int face)
{
    if (m_currentFace == std::max(0, face)) {
        return;
    }

    m_currentFace = std::max(0, face);
    Q_EMIT faceChanged();

    m_unicodeRangeChanged = true;
    polish();
}

unsigned FontPreviewItem::styleInfo() const
{
    return m_styleInfo;
}

void FontPreviewItem::setStyleInfo(unsigned value)
{
    if (m_styleInfo == value) {
        return;
    }

    m_styleInfo = value;
    Q_EMIT styleInfoChanged();

    polish();
}

FontPreviewItem::Mode FontPreviewItem::mode() const
{
    return m_mode;
}

void FontPreviewItem::setMode(FontPreviewItem::Mode newMode)
{
    if (m_mode == newMode) {
        return;
    }

    m_mode = newMode;

    Q_EMIT modeChanged();

    m_unicodeRangeChanged = true;
    polish();
}

QStringList FontPreviewItem::unicodeRangeNames() const
{
    return m_unicodeRangeNames;
}

int FontPreviewItem::unicodeRangeIndex() const
{
    return m_unicodeRangeIndex;
}

void FontPreviewItem::setUnicodeRangeIndex(int rangeId)
{
    if (m_unicodeRangeIndex == rangeId || rangeId < 0) {
        return;
    }

    m_unicodeRangeIndex = rangeId;
    Q_EMIT unicodeRangeChanged();

    m_unicodeRangeChanged = true;
    polish();
}

QString FontPreviewItem::previewText() const
{
    return m_previewString;
}

void FontPreviewItem::setPreviewText(const QString &text)
{
    if (m_previewString == text) {
        return;
    }

    m_previewString = text;
    Q_EMIT textChanged();

    polish();
}

int FontPreviewItem::fontPixelSize() const
{
    return m_fontPixelSize;
}

void FontPreviewItem::setFontPixelSize(int newSize)
{
    if (m_fontPixelSize == newSize) {
        return;
    }

    m_fontPixelSize = newSize;
    Q_EMIT pixelSizeChanged();

    polish();
}

void FontPreviewItem::paint(QPainter *painter)
{
    if (!m_ready || m_ftError || width() <= 0 || height() <= 0) {
        return;
    }

    painter->drawImage(QRect(QPoint(0, 0), size().toSize()), m_image);
}

void FontPreviewItem::updateBlockRange()
{
    if (!m_ready || (!m_unicodeRangeChanged && !m_ftError)) {
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

    // Update freetype font
    if (m_ftFace) {
        FT_Done_Face(m_ftFace);
    }

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
    m_ftError = FT_New_Face(m_ftLibrary, fontPath.constData(), m_currentFace, &m_ftFace);
    qDebug() << fontPath << m_ftError;
    if (m_ftError) {
        return;
    }

    m_characters.clear();

    switch (m_unicodeRangeIndex) {
    case 0: // Standard Preview
        // QString standardCharacters = QStringLiteral("abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ\n0123456789.:,;(*!?'/\\\")£$€%^&-+@~#<>{}[]");
        // Keep list empty and draw standard characters later
        break;

    case 1: { // All Characters
        FT_UInt index;
        unsigned charcode = FT_Get_First_Char(m_ftFace, &index);

        if (!index) {
            break;
        }

        m_characters.emplace_back(charcode);

        do {
            m_characters.emplace_back(FT_Get_Next_Char(m_ftFace, charcode, &index));
            // if FT_Get_Next_Char writes 0 to index then no more characters in font face
        } while (index);

        break;
    }

    default: {
        Q_ASSERT(m_unicodeRangeIndex >= 2); // Should be checked by setUnicodeRange
        if (m_unicodeRangeIndex < m_unicodeRangeNames.size()) {
            const auto &unicodeBlock = constUnicodeBlocks[m_unicodeRangeIndex - 2];
            for (unsigned charcode = unicodeBlock.start; charcode < unicodeBlock.end; ++charcode) {
                m_characters.emplace_back(charcode);
            }
        } else {
            int scriptIndex = m_unicodeRangeIndex - m_unicodeRangeNames.size();

            for (int i = 0; constUnicodeScripts[i].scriptIndex >= 0; ++i) {
                if (constUnicodeScripts[i].scriptIndex != scriptIndex) {
                    continue;
                }
                const auto &unicodeBlock = constUnicodeScripts[i];
                for (unsigned charcode = unicodeBlock.start; charcode < unicodeBlock.end; ++charcode) {
                    m_characters.emplace_back(charcode);
                }
            }
        }
    }
    }

    m_unicodeRangeChanged = false;
}

void FontPreviewItem::updatePolish()
{
    QQuickItem::updatePolish();

    if (!m_ready || width() <= 0 || height() <= 0) {
        // Need to wait until all properties are set
        return;
    }

    updateBlockRange();

    // Start generating font preview
    QRect usedRect;
    // BEGIN First line: descriptive name from freetype
    QString descriptiveName = KFI::fontDescriptiveName(m_fontName, m_currentFace);
    if (descriptiveName.isEmpty()) {
        descriptiveName = i18n("ERROR: Could not determine font's name.");
    }

    QFont defaultFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    defaultFont.setPointSize(10);
    const QFontMetrics defaultFontMetrics(defaultFont);
    QRect firstLineRect = defaultFontMetrics.boundingRect(descriptiveName);
    firstLineRect.moveTopLeft({0, 0});
    usedRect = firstLineRect;
    // END First line

    // BEGIN Second line: Draw selected characters, see https://stuff.mit.edu/afs/athena/astaff/source/src-9.0/third/freetype/docs/tutorial/step1.html
    // Set font size
    FT_Set_Pixel_Sizes(m_ftFace, 0, m_fontPixelSize);

    if (m_characters.empty()) {
        // Default characters
        static const std::string defaultChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:,;(*!?'/\\\")£$€%^&-+@~#<>{}[]";
        for (const char &c : defaultChars) {
            m_characters.emplace_back(c);
        }
    }
    // Convert charcode to glyph index in a font
    int currentX = 0;
    int currentY = usedRect.height();
    std::vector<std::pair<QRect, QImage>> charImageRecords;
    charImageRecords.reserve(std::min<std::size_t>(200, m_characters.size() + 9 * 43 /*lazy dog text*/));
    for (unsigned charcode : m_characters) {
        FT_Error ret = FT_Load_Char(m_ftFace, charcode, FT_LOAD_RENDER);
        if (ret) {
            continue;
        }
        // save the character width
        FT_GlyphSlot slot = m_ftFace->glyph;

        if (currentX + slot->bitmap.width <= width()) {
            QRect charRect(QPoint(currentX, currentY), QSize(slot->bitmap.width, slot->bitmap.rows));
            usedRect = usedRect.united(charRect);
            // Save image
            charImageRecords.emplace_back(std::make_pair(charRect, KFI::glyphToImage(slot)));
            // Move right
            currentX += slot->bitmap.width;
        } else if (currentY + slot->bitmap.rows <= height()) {
            // New line
            currentX = 0;
            currentY += slot->bitmap.rows;
        } else {
            // No more space
            break;
        }
    } // END Second line

    // BEGIN Third line: If there is still space, draw preview text
    if (m_unicodeRangeIndex == 0 && currentY < height() && !m_previewString.isEmpty()) {
        // Draw preview text or "The quick brown fox jumps over the lazy dog" in different font sizes
        constexpr std::array<short unsigned, 9> pixelSizes{4, 8, 12, 16, 20, 32, 36, 48, 64};
        for (short unsigned pixelSize : pixelSizes) {
            FT_Error ret = FT_Set_Pixel_Sizes(m_ftFace, 0, pixelSize);
            if (ret) {
                continue;
            }
            qDebug() << currentX << currentY << width() << height();

            currentX = 0;

            for (QChar singleChar : std::as_const(m_previewString)) {
                FT_Error ret = FT_Load_Char(m_ftFace, singleChar.unicode(), FT_LOAD_RENDER);
                if (ret) {
                    continue;
                }
                // save the character width
                FT_GlyphSlot slot = m_ftFace->glyph;
                if (currentX + slot->bitmap.width <= width()) {
                    QRect charRect(QPoint(currentX, currentY), QSize(slot->bitmap.width, slot->bitmap.rows));
                    usedRect = usedRect.united(charRect);
                    // Save image
                    auto it = charImageRecords.emplace_back(std::make_pair(charRect, KFI::glyphToImage(slot)));
                    // Move right
                    currentX += slot->bitmap.width;
                } else if (currentY + slot->bitmap.rows <= height()) {
                    // New line
                    currentX = 0;
                    currentY += slot->bitmap.rows;
                } else {
                    // No more space
                    break;
                }
            }
        }
    } // END Third line

    // BEGIN Draw all glyphs
    qDebug() << charImageRecords.size() << firstLineRect;
    m_image = QImage(size().toSize(), QImage::QImage::Format_Alpha8);
    QPainter p(&m_image);
    // Draw first line
    p.setFont(defaultFont);
    p.drawText(firstLineRect, Qt::AlignLeft, descriptiveName);

    if (!charImageRecords.empty()) {
        for (const auto &pair : charImageRecords) {
            p.drawImage(std::get<QRect>(pair), std::get<QImage>(pair));
        }
    }
    // END Draw all glyphs
    update();
}
