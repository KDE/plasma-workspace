#ifndef __FONT_VIEW_PART_H__
#define __FONT_VIEW_PART_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Family.h"
#include "FontPreview.h"
#include "KfiConstants.h"
#include "Misc.h"
#include <KParts/BrowserExtension>
#include <KParts/ReadOnlyPart>
#include <KSharedConfig>
#include <QFrame>
#include <QMap>
#include <QUrl>

class QPushButton;
class QLabel;
class QProcess;
class QAction;
class QSpinBox;
class QTemporaryDir;

namespace KFI
{
class BrowserExtension;
class FontInstInterface;

class CFontViewPart : public KParts::ReadOnlyPart
{
    Q_OBJECT

public:
    CFontViewPart(QWidget *parentWidget, QObject *parent, const QList<QVariant> &args);
    ~CFontViewPart() override;

    bool openUrl(const QUrl &url) override;

protected:
    bool openFile() override;

public Q_SLOTS:

    void previewStatus(bool st);
    void timeout();
    void install();
    void installlStatus();
    void dbusStatus(int pid, int status);
    void fontStat(int pid, const KFI::Family &font);
    void changeText();
    void print();
    void displayType(const QList<CFcEngine::TRange> &range);
    void showFace(int face);

private:
    void checkInstallable();

private:
    CFontPreview *itsPreview;
    QPushButton *itsInstallButton;
    QWidget *itsFaceWidget;
    QFrame *itsFrame;
    QLabel *itsFaceLabel;
    QSpinBox *itsFaceSelector;
    QAction *itsChangeTextAction;
    int itsFace;
    KSharedConfigPtr itsConfig;
    BrowserExtension *itsExtension;
    QProcess *itsProc;
    QTemporaryDir *itsTempDir;
    Misc::TFont itsFontDetails;
    FontInstInterface *itsInterface;
    bool itsOpening;
};

class BrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    BrowserExtension(CFontViewPart *parent);

    void enablePrint(bool enable);

public Q_SLOTS:

    void print();
};

}

#endif
