/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "quickfontpreviewitem.h"
#include "config-X11.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPalette>

#if HAVE_X11
#include <KWindowSystem>
#endif
#include <KLocalizedString>

#include "KfiConstants.h"
#include "UnicodeBlocks.h"
#include "UnicodeScripts.h"

namespace KFI
{
constexpr int constStepSize = 16;
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
}

FontPreviewItem::~FontPreviewItem()
{
}

void FontPreviewItem::componentComplete()
{
    QQuickItem::componentComplete();
    m_ready = true;
    updateBlockRange();
    polish();
}

QString FontPreviewItem::fontName() const
{
    return m_fontName;
}

void FontPreviewItem::setFontName(const QString &name)
{
    if (m_fontName == name) {
        return;
    }

    m_fontName = name;
    Q_EMIT nameChanged();

    m_image = QImage();
    polish();
}

int FontPreviewItem::fontFace() const
{
    return m_currentFace;
}

void FontPreviewItem::setFontFace(int face)
{
    if (m_currentFace == face) {
        return;
    }

    m_currentFace = face;
    Q_EMIT faceChanged();

    m_image = QImage();
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

    m_image = QImage();
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

    m_image = QImage();
    updateBlockRange();
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

    m_image = QImage();
    updateBlockRange();
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

    m_image = QImage();
    polish();
}

bool FontPreviewItem::isNull() const
{
    return m_image.isNull();
}

bool FontPreviewItem::atMax() const
{
    return m_engine ? m_engine->atMax() : true;
}

bool FontPreviewItem::atMin() const
{
    return m_engine ? m_engine->atMin() : true;
}

void FontPreviewItem::zoomIn()
{
    if (!m_engine || m_engine->atMax()) {
        return;
    }
    m_engine->zoomIn();
    m_image = QImage(); // To force update
    polish();
}

void FontPreviewItem::zoomOut()
{
    if (!m_engine || m_engine->atMin()) {
        return;
    }
    m_engine->zoomOut();
    m_image = QImage(); // To force update
    polish();
}

void FontPreviewItem::paint(QPainter *painter)
{
    if (!m_ready || width() <= 0 || height() <= 0) {
        return;
    }

    painter->drawImage(QRect(QPoint(0, 0), size().toSize()), m_image);
}

void FontPreviewItem::updateBlockRange()
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

    m_range.clear();

    switch (m_unicodeRangeIndex) {
    case 0:
        break;

    case 1:
        m_range.append(KFI::CFcEngine::TRange());
        break;

    default: {
        Q_ASSERT(m_unicodeRangeIndex >= 2); // Should be checked by setUnicodeRange
        if (m_unicodeRangeIndex < m_unicodeRangeNames.size()) {
            m_range.append(KFI::CFcEngine::TRange(constUnicodeBlocks[m_unicodeRangeIndex - 2].start, constUnicodeBlocks[m_unicodeRangeIndex - 2].end));
        } else {
            int scriptIndex = m_unicodeRangeIndex - m_unicodeRangeNames.size();

            for (int i = 0; constUnicodeScripts[i].scriptIndex >= 0; ++i) {
                if (constUnicodeScripts[i].scriptIndex == scriptIndex) {
                    m_range.append(KFI::CFcEngine::TRange(constUnicodeScripts[i].start, constUnicodeScripts[i].end));
                }
            }
        }
    }
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void FontPreviewItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
#else
void FontPreviewItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
#endif
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
#else
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
#endif
    polish();
}

void FontPreviewItem::updatePolish()
{
    QQuickItem::updatePolish();

    if (!m_ready || width() <= 0 || height() <= 0) {
        // Need to wait until all properties are set
        return;
    }

#if HAVE_X11
    if (!KWindowSystem::isPlatformX11()) {
        return;
    }
    if (!m_engine) {
        m_engine = std::make_unique<KFI::CFcEngine>();
    }
#else
    return; // FIXME Wayland is not supported by the engine
#endif

    // Now start generating font preview, same as CFontPreview::showFont()
    const bool oldIsNull = m_image.isNull();
    if (oldIsNull || std::abs(width() - m_lastWidth) > KFI::constStepSize || std::abs(height() - m_lastHeight) > KFI::constStepSize) {
        // Load/Reload preview
        m_lastWidth = width() + KFI::constStepSize;
        m_lastHeight = height() + KFI::constStepSize;
        m_engine->setPreviewString(m_previewString);
        m_image = m_engine->draw(m_fontName,
                                 m_styleInfo,
                                 m_currentFace,
                                 qGuiApp->palette().text().color(),
                                 qGuiApp->palette().base().color(),
                                 m_lastWidth,
                                 m_lastHeight,
                                 false,
                                 m_range,
                                 &m_chars);
        setAcceptHoverEvents(m_chars.count() > 0);
        m_lastChar = KFI::CFcEngine::TChar();
    }
    if (oldIsNull != m_image.isNull()) {
        Q_EMIT isNullChanged();
    }

    Q_EMIT atMaxChanged();
    Q_EMIT atMinChanged();

    update();
}
