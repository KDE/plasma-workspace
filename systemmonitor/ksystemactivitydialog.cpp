/*
    SPDX-FileCopyrightText: 2007-2010 John Tapsell <johnflux@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef Q_WS_WIN

#include "ksystemactivitydialog.h"

#include "processui/ksysguardprocesslist.h"

#include <QAction>
#include <QCloseEvent>
#include <QDBusConnection>
#include <QIcon>
#include <QLineEdit>
#include <QString>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KSharedConfig>
#include <KX11Extras>

KSystemActivityDialog::KSystemActivityDialog(QWidget *parent)
    : KMainWindow(parent)
    , m_configGroup(KSharedConfig::openConfig()->group("TaskDialog"))
{
    setAutoSaveSettings();

    m_processList = new KSysGuardProcessList;
    m_processList->setScriptingEnabled(true);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_processList);

    QWidget *mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);
    setCentralWidget(mainWidget);

    QAction *closeAction = new QAction;
    closeAction->setShortcuts({QKeySequence::Quit, Qt::Key_Escape});
    connect(closeAction, &QAction::triggered, this, &KSystemActivityDialog::close);
    addAction(closeAction);

    m_processList->loadSettings(m_configGroup);

    QDBusConnection con = QDBusConnection::sessionBus();
    con.registerObject(QStringLiteral("/"), this, QDBusConnection::ExportAllSlots);
}

void KSystemActivityDialog::run()
{
    show();
    raise();
    KX11Extras::setOnDesktop(winId(), KX11Extras::currentDesktop());
    KX11Extras::forceActiveWindow(winId());
}

void KSystemActivityDialog::setFilterText(const QString &filterText)
{
    m_processList->filterLineEdit()->setText(filterText);
    m_processList->filterLineEdit()->setFocus();
}

QString KSystemActivityDialog::filterText() const
{
    return m_processList->filterLineEdit()->text();
}

void KSystemActivityDialog::closeEvent(QCloseEvent *event)
{
    m_processList->saveSettings(m_configGroup);
    KSharedConfig::openConfig()->sync();

    KMainWindow::closeEvent(event);
}

QSize KSystemActivityDialog::sizeHint() const
{
    return QSize(650, 420);
}

#endif // not Q_WS_WIN
