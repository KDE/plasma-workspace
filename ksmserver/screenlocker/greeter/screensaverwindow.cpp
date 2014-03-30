/*
 *   Copyright 1999 Martin R. Jones <mjones@kde.org>
 *   Copyright 2003 Oswald Buddenhagen <ossi@kde.org>
 *   Copyright 2008 Chani Armitage <chanika@gmail.com>
 *   Copyright 2012 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "screensaverwindow.h"
#include "kscreensaversettings.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <QX11Info>

#include <KDebug>
#include <kmacroexpander.h>
#include <kshell.h>
#include <KService>
#include <KServiceTypeTrader>
#include <KAuthorized>
#include <KDesktopFile>
#include <KStandardDirs>

// X11
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <fixx11h.h>

namespace ScreenLocker
{

ScreenSaverWindow::ScreenSaverWindow(QWidget *parent)
    : QWidget(parent),
      m_startMousePos(-1, -1),
      m_forbidden(false),
      m_openGLVisual(false)
{
    setCursor(Qt::BlankCursor);
    m_reactivateTimer = new QTimer(this);
    m_reactivateTimer->setSingleShot(true);
    connect(m_reactivateTimer, SIGNAL(timeout()), this, SLOT(show()));

    setMouseTracking(true);
    m_saver = KScreenSaverSettings::saver();
    readSaver();
}

ScreenSaverWindow::~ScreenSaverWindow()
{
    stopXScreenSaver();
}

QPixmap ScreenSaverWindow::background() const
{
    return m_background;
}

void ScreenSaverWindow::setBackground(const QPixmap &pix)
{
    m_background = pix;
}

//---------------------------------------------------------------------------
//
// Read the command line needed to run the screensaver given a .desktop file.
//
void ScreenSaverWindow::readSaver()
{
    if (!m_saver.isEmpty())
    {
        QString entryName = m_saver;
        if( entryName.endsWith( QLatin1String( ".desktop" ) ))
            entryName = entryName.left( entryName.length() - 8 ); // strip it
        const KService::List offers = KServiceTypeTrader::self()->query( QLatin1String( "ScreenSaver" ),
            QLatin1String( "DesktopEntryName == '" ) + entryName.toLower() + QLatin1Char( '\'' ) );
        if( offers.isEmpty() )
        {
            kDebug() << "Cannot find screesaver: " << m_saver;
            return;
        }
        const QString file = KStandardDirs::locate("services", offers.first()->entryPath());

        const bool opengl = KAuthorized::authorizeKAction(QLatin1String( "opengl_screensavers" ));
        const bool manipulatescreen = KAuthorized::authorizeKAction(QLatin1String( "manipulatescreen_screensavers" ));
        KDesktopFile config( file );
        KConfigGroup desktopGroup = config.desktopGroup();
        foreach (const QString &type, desktopGroup.readEntry("X-KDE-Type").split(QLatin1Char(';'))) {
            if (type == QLatin1String("ManipulateScreen")) {
                if (!manipulatescreen) {
                    kDebug() << "Screensaver is type ManipulateScreen and ManipulateScreen is forbidden";
                    m_forbidden = true;
                }
            } else if (type == QLatin1String("OpenGL")) {
                m_openGLVisual = true;
                if (!opengl) {
                    kDebug() << "Screensaver is type OpenGL and OpenGL is forbidden";
                    m_forbidden = true;
                }
            }
        }

        kDebug() << "m_forbidden: " << (m_forbidden ? "true" : "false");

        if (config.hasActionGroup(QLatin1String( "InWindow" )))
        {
            m_saverExec = config.actionGroup(QLatin1String( "InWindow" )).readPathEntry("Exec", QString());
        }
    }
}

void ScreenSaverWindow::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    m_startMousePos = QPoint(-1, -1);
    //reappear in one minute
    m_reactivateTimer->start(1000 * 60);
    hide();
    emit hidden();
}


void ScreenSaverWindow::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event)

    hide();
    emit hidden();
}

void ScreenSaverWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_startMousePos == QPoint(-2, -2)) {
        m_startMousePos = QPoint(-1, -1); // reset
        return; // caused by show event
    }
    if (m_startMousePos == QPoint(-1, -1)) {
        m_startMousePos = event->globalPos();
    }
    else if ((event->globalPos() - m_startMousePos).manhattanLength() > QApplication::startDragDistance()) {
        m_startMousePos = QPoint(-1, -1);
        hide();
        emit hidden();
        //reappear in one minute
        m_reactivateTimer->start(1000 * 60);
    }
}

void ScreenSaverWindow::showEvent(QShowEvent *event)
{
    m_startMousePos = QPoint(-2, -2); // prevent mouse interpretation to cause an immediate hide
    m_reactivateTimer->stop();
    static Atom tag = XInternAtom(QX11Info::display(), "_KDE_SCREEN_LOCKER", False);
    if (testAttribute(Qt::WA_WState_Created) && internalWinId()) {
        XChangeProperty(QX11Info::display(), winId(), tag, tag, 32, PropModeReplace, 0, 0);
    }
    startXScreenSaver();
}

void ScreenSaverWindow::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.drawPixmap(m_background.rect(), m_background, m_background.rect());
    p.end();
}

//---------------------------------------------------------------------------
//


bool ScreenSaverWindow::startXScreenSaver()
{
    //QString m_saverExec("kannasaver.kss --window-id=%w");
    kDebug() << "Starting hack:" << m_saverExec;

    if (m_saverExec.isEmpty() || m_forbidden) {
        return false;
    }

    QHash<QChar, QString> keyMap;
    keyMap.insert(QLatin1Char( 'w' ), QString::number(winId()));
    m_ScreenSaverProcess << KShell::splitArgs(KMacroExpander::expandMacrosShellQuote(m_saverExec, keyMap));

    m_ScreenSaverProcess.start();
    if (m_ScreenSaverProcess.waitForStarted()) {
#ifdef HAVE_SETPRIORITY
        setpriority(PRIO_PROCESS, m_ScreenSaverProcess.pid(), mPriority);
#endif
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------
//
void ScreenSaverWindow::stopXScreenSaver()
{
    if (m_ScreenSaverProcess.state() != QProcess::NotRunning) {
        m_ScreenSaverProcess.terminate();
        if (!m_ScreenSaverProcess.waitForFinished(10000)) {
            m_ScreenSaverProcess.kill();
        }
    }
}

} // end namespace
#include "screensaverwindow.moc"
