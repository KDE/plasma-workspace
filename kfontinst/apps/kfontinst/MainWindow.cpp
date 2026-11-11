/*
 * SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MainWindow.h"
#include "FontManager.h"

#include <KActionCollection>
#include <KStandardActions>

#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

using namespace Qt::StringLiterals;

namespace KFI
{
MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    setCentralWidget(new CFontManager(this));

    KStandardActions::quit(qApp, &QApplication::closeAllWindows, actionCollection());

    setupGUI(QSize(900, 650), Keys | Save | Create);

    // The handbook still lives in the documentation of the former System Settings module
    if (const auto handbook = actionCollection()->action(KStandardActions::name(KStandardActions::HelpContents))) {
        disconnect(handbook, &QAction::triggered, nullptr, nullptr);
        connect(handbook, &QAction::triggered, this, [] {
            QDesktopServices::openUrl(QUrl(u"help:/kcontrol/fontinst/index.html"_s));
        });
    }
}

MainWindow::~MainWindow() = default;

}

#include "moc_MainWindow.cpp"
