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

#include <QAbstractScrollArea>
#include <QCloseEvent>
#include <QLineEdit>
#include <QLayout>
#include <QString>
#include <QAction>
#include <QTreeView>

#include <KConfigGroup>
#include <KGlobal>
#include <KWindowSystem>
#include <KIcon>

#include <QDBusConnection>

//#include "krunnersettings.h"

KSystemActivityDialog::KSystemActivityDialog(QWidget *parent)
    : KDialog(parent), m_processList(0)
{
    setWindowTitle(i18n("System Activity"));
    setWindowIcon(KIcon(QLatin1String( "utilities-system-monitor" )));
    setButtons(0);
    setMainWidget(&m_processList);
    m_processList.setScriptingEnabled(true);
    setSizeGripEnabled(true);
    (void)minimumSizeHint(); //Force the dialog to be laid out now
    layout()->setContentsMargins(0,0,0,0);
    m_processList.treeView()->setCornerWidget(new QWidget);

    // Since we kinda act like an application more than just a Window, map the usual ctrl+Q shortcut to close as well
    QAction *closeWindow = new QAction(this);
    closeWindow->setShortcut(QKeySequence::Quit);
    connect(closeWindow, SIGNAL(triggered(bool)), this, SLOT(accept()));
    addAction(closeWindow);

    setInitialSize(QSize(650, 420));
    KConfigGroup cg = KGlobal::config()->group("TaskDialog");
    restoreDialogSize(cg);

    m_processList.loadSettings(cg);
    // Since we default to forcing the window to be KeepAbove, if the user turns this off, remember this
    const bool keepAbove = true; // KRunnerSettings::keepTaskDialogAbove();
    if (keepAbove) {
        KWindowSystem::setState(winId(), NET::KeepAbove );
    }

    QDBusConnection con = QDBusConnection::sessionBus();
    con.registerObject(QStringLiteral("/"), this, QDBusConnection::ExportAllSlots);
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
    KConfigGroup cg = KGlobal::config()->group("TaskDialog");
    saveDialogSize(cg);
    m_processList.saveSettings(cg);

    // Since we default to forcing the window to be KeepAbove, if the user turns this off, remember this
    // vHanda: Temporarily commented out
    // bool keepAbove = KWindowSystem::windowInfo(winId(), NET::WMState).hasState(NET::KeepAbove);
    // KRunnerSettings::setKeepTaskDialogAbove(keepAbove);
    KGlobal::config()->sync();
}

#endif // not Q_WS_WIN

