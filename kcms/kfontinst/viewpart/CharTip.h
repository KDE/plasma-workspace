#ifndef __CHAR_TIP_H__
#define __CHAR_TIP_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FcEngine.h"
#include <QFrame>

class QLabel;
class QTimer;
class QResizeEvent;
class QEvent;

namespace KFI
{
class CFontPreview;

class CCharTip : public QFrame
{
    Q_OBJECT

public:
    CCharTip(CFontPreview *parent);
    ~CCharTip() override;

    void setItem(const CFcEngine::TChar &ch);

private Q_SLOTS:

    void showTip();
    void hideTip();

private:
    void reposition();
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *, QEvent *e) override;

private:
    CFontPreview *itsParent;
    QLabel *itsLabel, *itsPixmapLabel;
    QTimer *itsTimer;
    CFcEngine::TChar itsItem;
};

}

#endif
