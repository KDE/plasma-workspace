/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QImage>
#include <QQuickPaintedItem>

#include <freetype/freetype.h>

/**
 * Provides a preview area for fonts that supports zoom in/out
 *
 * @see KFI::CFontPreview
 */
class FontPreviewItem : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QString name READ fontName WRITE setFontName NOTIFY nameChanged)
    Q_PROPERTY(int face READ fontFace WRITE setFontFace NOTIFY faceChanged)
    Q_PROPERTY(unsigned styleInfo READ styleInfo WRITE setStyleInfo NOTIFY styleInfoChanged)

    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(int unicodeRangeIndex READ unicodeRangeIndex WRITE setUnicodeRangeIndex NOTIFY unicodeRangeChanged)
    Q_PROPERTY(QStringList unicodeRangeNames READ unicodeRangeNames NOTIFY unicodeRangeNamesChanged)

    Q_PROPERTY(QString text READ previewText WRITE setPreviewText NOTIFY textChanged)
    Q_PROPERTY(int pixelSize READ fontPixelSize WRITE setFontPixelSize NOTIFY pixelSizeChanged)

public:
    enum Mode {
        Basic,
        BlocksAndScripts,
        ScriptsOnly,
    };
    Q_ENUM(Mode)

    explicit FontPreviewItem(QQuickItem *parent = nullptr);
    ~FontPreviewItem() override;

    void componentComplete() override;

    QString fontName() const;
    void setFontName(const QString &name);

    int fontFace() const;
    void setFontFace(int face);

    unsigned styleInfo() const;
    void setStyleInfo(unsigned value);

    Mode mode() const;
    /**
     * @see KFI::CPreviewSelectAction::setMode
     */
    void setMode(Mode newMode);

    int unicodeRangeIndex() const;
    void setUnicodeRangeIndex(int rangeId);

    QStringList unicodeRangeNames() const;

    QString previewText() const;
    void setPreviewText(const QString &text);

    int fontPixelSize() const;
    void setFontPixelSize(int newSize);

    void paint(QPainter *painter) override;

Q_SIGNALS:
    void nameChanged();
    void faceChanged();
    void styleInfoChanged();
    void modeChanged();
    void unicodeRangeChanged();
    void unicodeRangeNamesChanged();
    void textChanged();
    void pixelSizeChanged();

protected:
    void updatePolish() override;

private:
    void showFont();

    /**
     * @see KFI::CPreviewSelectAction::selected
     */
    void updateBlockRange();

    bool m_ready = false;
    QString m_fontName;
    int m_currentFace = 1;
    int m_lastWidth = 0;
    int m_lastHeight = 0;
    unsigned m_styleInfo;

    QImage m_image;

    Mode m_mode = Basic;
    QStringList m_unicodeRangeNames;
    int m_unicodeRangeIndex = 0;
    FT_Error m_ftError = 0;
    FT_Library m_ftLibrary = nullptr;
    FT_Face m_ftFace = nullptr;
    std::vector<unsigned> m_characters;
    bool m_unicodeRangeChanged = true;

    QString m_previewString;
    int m_fontPixelSize = 10;
};
