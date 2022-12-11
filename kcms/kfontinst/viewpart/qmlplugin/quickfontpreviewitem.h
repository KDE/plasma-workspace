/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QImage>
#include <QQuickPaintedItem>

#include "FcEngine.h"

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

    Q_PROPERTY(bool isNull READ isNull NOTIFY isNullChanged)
    Q_PROPERTY(bool atMax READ atMax NOTIFY atMaxChanged)
    Q_PROPERTY(bool atMin READ atMin NOTIFY atMinChanged)

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
    void paint(QPainter *painter) override;

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

    bool isNull() const;

    bool atMax() const;
    bool atMin() const;

    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();

Q_SIGNALS:
    void nameChanged();
    void faceChanged();
    void styleInfoChanged();
    void modeChanged();
    void unicodeRangeChanged();
    void unicodeRangeNamesChanged();
    void textChanged();

    /**
     * Emitted when font preview becomes ready or invalid
     */
    void isNullChanged();

    void atMaxChanged();
    void atMinChanged();

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif
    void updatePolish() override;

private:
    /**
     * @see KFI::CPreviewSelectAction::selected
     */
    void updateBlockRange();

    std::unique_ptr<KFI::CFcEngine> m_engine;

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
    QList<KFI::CFcEngine::TRange> m_range;
    QList<KFI::CFcEngine::TChar> m_chars;
    KFI::CFcEngine::TChar m_lastChar;

    QString m_previewString;
};
