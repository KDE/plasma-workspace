/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ActionLabel.h"
#include <KIconLoader>
#include <QPixmap>
#include <QTimer>
#include <QTransform>

namespace KFI
{
// Borrowed from kolourpaint...
static QTransform matrixWithZeroOrigin(const QTransform &matrix, int width, int height)
{
    QRect newRect(matrix.mapRect(QRect(0, 0, width, height)));

    return QTransform(matrix.m11(), matrix.m12(), matrix.m21(), matrix.m22(), matrix.dx() - newRect.left(), matrix.dy() - newRect.top());
}

static QTransform rotateMatrix(int width, int height, double angle)
{
    QTransform matrix;
    matrix.translate(width / 2.0, height / 2.0);
    matrix.rotate(angle);

    return matrixWithZeroOrigin(matrix, width, height);
}

static const int constNumIcons = 8;
static int theUsageCount;
static QPixmap *theIcons[constNumIcons];

CActionLabel::CActionLabel(QWidget *parent)
    : QLabel(parent)
{
    static const int constIconSize(48);

    setMinimumSize(constIconSize, constIconSize);
    setMaximumSize(constIconSize, constIconSize);
    setAlignment(Qt::AlignCenter);

    if (0 == theUsageCount++) {
        QImage img(KIconLoader::global()->loadIcon("application-x-font-ttf", KIconLoader::NoGroup, 32).toImage());
        double increment = 360.0 / constNumIcons;

        for (int i = 0; i < constNumIcons; ++i) {
            theIcons[i] = new QPixmap(QPixmap::fromImage(0 == i ? img : img.transformed(rotateMatrix(img.width(), img.height(), increment * i))));
        }
    }

    setPixmap(*theIcons[0]);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CActionLabel::rotateIcon);
}

CActionLabel::~CActionLabel()
{
    if (0 == --theUsageCount) {
        for (int i = 0; i < constNumIcons; ++i) {
            delete theIcons[i];
            theIcons[i] = nullptr;
        }
    }
}

void CActionLabel::startAnimation()
{
    m_count = 0;
    setPixmap(*theIcons[0]);
    m_timer->start(1000 / constNumIcons);
}

void CActionLabel::stopAnimation()
{
    m_timer->stop();
    m_count = 0;
    setPixmap(*theIcons[m_count]);
}

void CActionLabel::rotateIcon()
{
    if (++m_count == constNumIcons) {
        m_count = 0;
    }

    setPixmap(*theIcons[m_count]);
}

}
