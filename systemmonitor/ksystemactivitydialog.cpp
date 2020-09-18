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
#include <QString>
#include <QAction>
#include <QIcon>
#include <QDBusConnection>
#include <QVBoxLayout>
#include <QLineEdit>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KWindowSystem>
#include <KLocalizedString>
#include <QDebug>

KSystemActivityDialog::KSystemActivityDialog(QWidget *parent)
    : QDialog(parent), m_processList(nullptr)
{
    setWindowTitle(i18n("System Activity"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral( "utilities-system-monitor" )));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(&m_processList);
    m_processList.setScriptingEnabled(true);
    setSizeGripEnabled(true);

    // Since we kinda act like an application more than just a Window, map the usual ctrl+Q shortcut to close as well
    QAction *closeWindow = new QAction(this);
    closeWindow->setShortcut(QKeySequence::Quit);
    connect(closeWindow, &QAction::triggered, this, &KSystemActivityDialog::accept);
    addAction(closeWindow);

    KConfigGroup cg = KSharedConfig::openConfig()->group("TaskDialog");
    m_processList.loadSettings(cg);

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

void KSystemActivityDialog::reject()
{
    saveDialogSettings();
    QDialog::reject();
}

void KSystemActivityDialog::saveDialogSettings()
{
    // When the user closes the dialog, save the process list setup
    KConfigGroup cg = KSharedConfig::openConfig()->group("TaskDialog");
    m_processList.saveSettings(cg);
    KSharedConfig::openConfig()->sync();
}

QSize KSystemActivityDialog::sizeHint() const
{
    return QSize(650, 420);
}

#endif // not Q_WS_WIN
