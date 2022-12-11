/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QQmlParserStatus>

#include <freetype/freetype.h>

/**
 * Provides a preview area for fonts that supports zoom in/out
 *
 * @see KFI::CFontPreview
 */
class FontPreviewBackend : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QString name READ fontName WRITE setFontName NOTIFY nameChanged)
    Q_PROPERTY(int face READ fontFace WRITE setFontFace NOTIFY faceChanged)
    Q_PROPERTY(unsigned previewCount READ previewCount WRITE setPreviewCount NOTIFY previewCountChanged)

    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(int unicodeRangeIndex READ unicodeRangeIndex WRITE setUnicodeRangeIndex NOTIFY unicodeRangeChanged)
    Q_PROPERTY(QStringList unicodeRangeNames READ unicodeRangeNames NOTIFY unicodeRangeNamesChanged)

    Q_PROPERTY(QString characters READ characters NOTIFY charactersChanged)

public:
    enum Mode {
        Basic,
        BlocksAndScripts,
        ScriptsOnly,
    };
    Q_ENUM(Mode)

    explicit FontPreviewBackend(QObject *parent = nullptr);
    ~FontPreviewBackend() override;

    void classBegin() override;
    void componentComplete() override;

    QString fontName() const;
    void setFontName(const QString &name);

    int fontFace() const;
    void setFontFace(int id);

    unsigned previewCount() const;
    void setPreviewCount(unsigned value);

    Mode mode() const;
    /**
     * @see KFI::CPreviewSelectAction::setMode
     */
    void setMode(Mode newMode);

    int unicodeRangeIndex() const;
    void setUnicodeRangeIndex(int rangeId);

    QStringList unicodeRangeNames() const;

    QString characters() const;

Q_SIGNALS:
    void nameChanged();
    void faceChanged();
    void previewCountChanged();
    void modeChanged();
    void unicodeRangeChanged();
    void unicodeRangeNamesChanged();
    void charactersChanged();

private:
    /**
     * @see KFI::CPreviewSelectAction::selected
     */
    void updateBlockRange();

    bool m_ready = false;
    QString m_fontName;
    QString m_styleName;
    int m_fontFace = 1;

    unsigned m_previewCount = 0;

    Mode m_mode = Basic;
    QStringList m_unicodeRangeNames;
    int m_unicodeRangeIndex = 0;
    QString m_characters;

    FT_Error m_ftError = 0;
    FT_Library m_ftLibrary = nullptr;
    FT_Face m_ftFace = nullptr;

    QFutureWatcher<QString> *m_futureWatcher = nullptr;
};
