/*
 *   Copyright (C) 2007-2010 John Tapsell <johnflux@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef Q_WS_WIN

#include "ksystemactivitydialog.h"

#include <QCloseEvent>
#include <QLayout>
#include <QString>
#include <QAction>
#include <QTreeView>
#include <QIcon>
#include <QWindow>
#include <QDialog>
#include <QDBusConnection>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QScreen>
#include <QDesktopWidget>
#include <QTimer>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KWindowConfig>
#include <KWindowSystem>
#include <KLocalizedString>
#include <QDebug>

KSystemActivityDialog::KSystemActivityDialog(QWidget *parent)
    : QDialog(parent), m_processList(0)
{
    setWindowTitle(i18n("System Activity"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral( "utilities-system-monitor" )));
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(&m_processList);
    m_processList.setScriptingEnabled(true);
    setSizeGripEnabled(true);
    (void)minimumSizeHint(); //Force the dialog to be laid out now
    layout()->setContentsMargins(0,0,0,0);

    // Since we kinda act like an application more than just a Window, map the usual ctrl+Q shortcut to close as well
    QAction *closeWindow = new QAction(this);
    closeWindow->setShortcut(QKeySequence::Quit);
    connect(closeWindow, &QAction::triggered, this, &KSystemActivityDialog::accept);
    addAction(closeWindow);

    // We need the resizing to be done once the dialog has been initialized
    // otherwise we don't actually have a window.
    QTimer::singleShot(0, this, &KSystemActivityDialog::slotInit);
}

void KSystemActivityDialog::slotInit()
{
    resize(QSize(650, 420));

    KConfigGroup cg = KSharedConfig::openConfig()->group("TaskDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), cg);

    m_processList.loadSettings(cg);
    // Since we default to forcing the window to be KeepAbove, if the user turns this off, remember this
    const bool keepAbove = true; // KRunnerSettings::keepTaskDialogAbove();
    if (keepAbove) {
        KWindowSystem::setState(winId(), NET::KeepAbove);
    }

    QDBusConnection con = QDBusConnection::sessionBus();
    con.registerObject(QStringLiteral("/"), this, QDBusConnection::ExportAllSlots);

    QRect geom = windowHandle()->screen()->geometry();
    QSize ourSize = windowHandle()->size();

    int w = ourSize.width();
    int h = ourSize.height();
    setGeometry((geom.width() - w)/2, (geom.height() - h)/2, w, h);
}

void KSystemActivityDialog::run()
{
    show();
    raise();
    KWindowSystem::setOnDesktop(winId(), KWindowSystem::currentDesktop());
    KWindowSystem::forceActiveWindow(winId());
}

void KSystemActivityDialog::setFilterText(const QString &filterText)
{
    m_processList.filterLineEdit()->setText(filterText);
    m_processList.filterLineEdit()->setFocus();
}

QString KSystemActivityDialog::filterText() const
{
    return m_processList.filterLineEdit()->text();
}

void KSystemActivityDialog::closeEvent(QCloseEvent *event)
{
    saveDialogSettings();
    event->accept();
}


void KSystemActivityDialog::reject ()
{
    saveDialogSettings();
    QDialog::reject();
}

void KSystemActivityDialog::saveDialogSettings()
{
    //When the user closes the dialog, save the position and the KeepAbove state
    KConfigGroup cg = KSharedConfig::openConfig()->group("TaskDialog");
    KWindowConfig::saveWindowSize(windowHandle(), cg);
    m_processList.saveSettings(cg);

    // Since we default to forcing the window to be KeepAbove, if the user turns this off, remember this
    // vHanda: Temporarily commented out
    // bool keepAbove = KWindowSystem::windowInfo(winId(), NET::WMState).hasState(NET::KeepAbove);
    // KRunnerSettings::setKeepTaskDialogAbove(keepAbove);
    KSharedConfig::openConfig()->sync();
}

#endif // not Q_WS_WIN

