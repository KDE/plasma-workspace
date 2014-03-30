//===========================================================================
//
// This file is part of the KDE project
//
// Copyright 2004 Chris Howells <howells@kde.org>

#include "autologout.h"
#include "lockwindow.h"

#include <kconfig.h>
#include <kmessagebox.h>
#include <KLocalizedString>
#include <kdialog.h>
#include <KIconLoader>
#include <ksmserver_interface.h>

#include <QLayout>
#include <QMessageBox>
#include <QLabel>
#include <QStyle>
#include <QApplication>
#include <QDialog>
#include <QAbstractEventDispatcher>
#include <QProgressBar>
#include <QtDBus>

#define COUNTDOWN 30

AutoLogout::AutoLogout(ScreenLocker::LockWindow *parent) : QDialog(NULL, Qt::X11BypassWindowManagerHint)
{
    QLabel *pixLabel = new QLabel( this );
    pixLabel->setObjectName( QLatin1String( "pixlabel" ) );
    pixLabel->setPixmap(DesktopIcon(QLatin1String( "application-exit" )));

    QLabel *greetLabel = new QLabel(i18n("<qt><nobr><b>Automatic Log Out</b></nobr></qt>"), this);
    QLabel *infoLabel = new QLabel(i18n("<qt>To prevent being logged out, resume using this session by moving the mouse or pressing a key.</qt>"), this);

    mStatusLabel = new QLabel(QLatin1String( "<b> </b>" ), this);
    mStatusLabel->setAlignment(Qt::AlignCenter);

    QLabel *mProgressLabel = new QLabel(i18n("Time Remaining:"), this);
    mProgressRemaining = new QProgressBar(this);
    mProgressRemaining->setTextVisible(false);

    frameLayout = new QGridLayout(this);
    frameLayout->setSpacing(KDialog::spacingHint());
    frameLayout->setMargin(KDialog::marginHint() * 2);
    frameLayout->addWidget(pixLabel, 0, 0, 3, 1, Qt::AlignCenter | Qt::AlignTop);
    frameLayout->addWidget(greetLabel, 0, 1);
    frameLayout->addWidget(mStatusLabel, 1, 1);
    frameLayout->addWidget(infoLabel, 2, 1);
    frameLayout->addWidget(mProgressLabel, 3, 1);
    frameLayout->addWidget(mProgressRemaining, 4, 1);

    // get the time remaining in seconds for the status label
    mRemaining = COUNTDOWN * 25;

    mProgressRemaining->setMaximum(COUNTDOWN * 25);

    updateInfo(mRemaining);

    mCountdownTimerId = startTimer(1000/25);

    connect(parent, SIGNAL(userActivity()), SLOT(slotActivity()));
}

AutoLogout::~AutoLogout()
{
    hide();
}

void AutoLogout::updateInfo(int timeout)
{
    mStatusLabel->setText(i18np("<qt><nobr>You will be automatically logged out in 1 second</nobr></qt>",
                               "<qt><nobr>You will be automatically logged out in %1 seconds</nobr></qt>",
                               timeout / 25) );
    mProgressRemaining->setValue(timeout);
}

void AutoLogout::timerEvent(QTimerEvent *ev)
{
    if (ev->timerId() == mCountdownTimerId)
    {
        updateInfo(mRemaining);
        --mRemaining;
        if (mRemaining < 0)
        {
            killTimer(mCountdownTimerId);
            logout();
        }
    }
}

void AutoLogout::slotActivity()
{
    if (mRemaining >= 0)
        accept();
}

void AutoLogout::logout()
{
    QAbstractEventDispatcher::instance()->unregisterTimers(this);
    org::kde::KSMServerInterface ksmserver(QLatin1String( "org.kde.ksmserver" ), QLatin1String( "/KSMServer" ), QDBusConnection::sessionBus());
    ksmserver.logout( 0, 0, 0 );
}

void AutoLogout::setVisible(bool visible)
{
    QDialog::setVisible(visible);

    if (visible)
        QApplication::flush();
}

#include "autologout.moc"
