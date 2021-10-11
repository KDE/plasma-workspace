#pragma once

/*
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
    CFontPreview *m_preview;
    QPushButton *m_installButton;
    QWidget *m_faceWidget;
    QFrame *m_frame;
    QLabel *m_faceLabel;
    QSpinBox *m_faceSelector;
    QAction *m_changeTextAction;
    int m_face;
    KSharedConfigPtr m_config;
    BrowserExtension *m_extension;
    QProcess *m_proc;
    QTemporaryDir *m_tempDir;
    Misc::TFont m_fontDetails;
    FontInstInterface *m_interface;
    bool m_opening;
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
