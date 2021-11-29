/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "FontPreview.h"
#include "CharTip.h"
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <stdlib.h>

namespace KFI
{
static const int constBorder = 4;
static const int constStepSize = 16;

CFontPreview::CFontPreview(QWidget *parent)
    : QWidget(parent)
    , m_currentFace(1)
    , m_lastWidth(0)
    , m_lastHeight(0)
    , m_styleInfo(KFI_NO_STYLE_INFO)
    , m_tip(nullptr)
{
    m_engine = new CFcEngine;
}

CFontPreview::~CFontPreview()
{
    delete m_tip;
    delete m_engine;
}

void CFontPreview::showFont(const QString &name, unsigned long styleInfo, int face)
{
    m_fontName = name;
    m_styleInfo = styleInfo;
    showFace(face);
}

void CFontPreview::showFace(int face)
{
    m_currentFace = face;
    showFont();
}

void CFontPreview::showFont()
{
    m_lastWidth = width() + constStepSize;
    m_lastHeight = height() + constStepSize;

    m_image = m_engine->draw(m_fontName,
                             m_styleInfo,
                             m_currentFace,
                             palette().text().color(),
                             palette().base().color(),
                             m_lastWidth,
                             m_lastHeight,
                             false,
                             m_range,
                             &m_chars);

    if (!m_image.isNull()) {
        m_lastChar = CFcEngine::TChar();
        setMouseTracking(m_chars.count() > 0);
        update();
        Q_EMIT status(true);
        Q_EMIT atMax(m_engine->atMax());
        Q_EMIT atMin(m_engine->atMin());
    } else {
        m_lastChar = CFcEngine::TChar();
        setMouseTracking(false);
        update();
        Q_EMIT status(false);
        Q_EMIT atMax(true);
        Q_EMIT atMin(true);
    }
}

void CFontPreview::zoomIn()
{
    m_engine->zoomIn();
    showFont();
    Q_EMIT atMax(m_engine->atMax());
}

void CFontPreview::zoomOut()
{
    m_engine->zoomOut();
    showFont();
    Q_EMIT atMin(m_engine->atMin());
}

void CFontPreview::setUnicodeRange(const QList<CFcEngine::TRange> &r)
{
    m_range = r;
    showFont();
}

void CFontPreview::paintEvent(QPaintEvent *)
{
    QPainter paint(this);

    paint.fillRect(rect(), palette().base());
    if (!m_image.isNull()) {
        if (abs(width() - m_lastWidth) > constStepSize || abs(height() - m_lastHeight) > constStepSize) {
            showFont();
        } else {
            paint.drawImage(
                QPointF(constBorder, constBorder),
                m_image,
                QRectF(0, 0, (width() - (constBorder * 2)) * m_image.devicePixelRatioF(), (height() - (constBorder * 2)) * m_image.devicePixelRatioF()));
        }
    }
}

void CFontPreview::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_chars.isEmpty()) {
        QList<CFcEngine::TChar>::ConstIterator end(m_chars.end());

        if (m_lastChar.isNull() || !m_lastChar.contains(event->pos())) {
            for (QList<CFcEngine::TChar>::ConstIterator it(m_chars.begin()); it != end; ++it) {
                if ((*it).contains(event->pos())) {
                    if (!m_tip) {
                        m_tip = new CCharTip(this);
                    }

                    m_tip->setItem(*it);
                    m_lastChar = *it;
                    break;
                }
            }
        }
    }
}

void CFontPreview::wheelEvent(QWheelEvent *e)
{
    if (e->angleDelta().y() > 0) {
        zoomIn();
    } else if (e->angleDelta().y() < 0) {
        zoomOut();
    }

    e->accept();
}

QSize CFontPreview::sizeHint() const
{
    return QSize(132, 132);
}

QSize CFontPreview::minimumSizeHint() const
{
    return QSize(32, 32);
}

}
