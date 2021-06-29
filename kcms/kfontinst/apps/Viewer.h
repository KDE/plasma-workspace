#ifndef __VIEWER_H__
#define __VIEWER_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KParts/MainWindow>
#include <KParts/ReadOnlyPart>

class QAction;
class QUrl;

namespace KFI
{
class CViewer : public KParts::MainWindow
{
    Q_OBJECT

public:
    CViewer();
    ~CViewer() override
    {
    }
    void showUrl(const QUrl &url);

public Q_SLOTS:

    void fileOpen();
    void configureKeys();
    void enableAction(const char *name, bool enable);

private:
    KParts::ReadOnlyPart *itsPreview;
    QAction *itsPrintAct;
};

}

#endif
