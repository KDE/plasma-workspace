#ifndef __ACTION_LABEL_H__
#define __ACTION_LABEL_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QLabel>

class QTimer;
class QLabel;

namespace KFI
{
class CActionLabel : public QLabel
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
    QTimer *itsTimer;
    int itsCount;
};

}

#endif
