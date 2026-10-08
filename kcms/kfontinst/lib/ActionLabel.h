/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kfontinst_export.h"
#include <QLabel>

class QTimer;

namespace KFI
{
class KFONTINST_EXPORT CActionLabel : public QLabel
{
    Q_OBJECT

public:
    CActionLabel(QWidget *parent);
    ~CActionLabel() override;
    void startAnimation();
    void stopAnimation();

private Q_SLOTS:

    void rotateIcon();

protected:
    QTimer *m_timer;
    int m_count;
};

}
