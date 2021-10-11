#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FcEngine.h"
#include "KfiConstants.h"
#include <QImage>
#include <QPaintEvent>
#include <QSize>
#include <QWidget>

class QWheelEvent;

namespace KFI
{
class CCharTip;
class CFcEngine;

class CFontPreview : public QWidget
{
    Q_OBJECT

public:
    CFontPreview(QWidget *parent);
    ~CFontPreview() override;

    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *e) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void showFont(const QString &name, // Thsi is either family name, or filename
                  unsigned long styleInfo = KFI_NO_STYLE_INFO,
                  int face = 0);
    void showFont();
    void showFace(int face);

    CFcEngine *engine()
    {
        return m_engine;
    }

public Q_SLOTS:

    void setUnicodeRange(const QList<CFcEngine::TRange> &r);
    void zoomIn();
    void zoomOut();

Q_SIGNALS:

    void status(bool);
    void atMax(bool);
    void atMin(bool);

private:
    QImage m_image;
    int m_currentFace, m_lastWidth, m_lastHeight, m_styleInfo;
    QString m_fontName;
    QList<CFcEngine::TRange> m_range;
    QList<CFcEngine::TChar> m_chars;
    CFcEngine::TChar m_lastChar;
    CCharTip *m_tip;
    CFcEngine *m_engine;

    friend class CCharTip;
};

}
