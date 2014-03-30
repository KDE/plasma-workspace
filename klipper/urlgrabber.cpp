/* This file is part of the KDE project
   Copyright (C) (C) 2000,2001,2002 by Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "urlgrabber.h"

#include <netwm.h>

#include <QHash>
#include <QTimer>
#include <QUuid>
#include <QFile>
#include <QX11Info>

#include <KDialog>
#include <KLocalizedString>
#include <KMenu>
#include <KService>
#include <KDebug>
#include <KIconLoader>
#include <KStringHandler>
#include <KGlobal>
#include <KMimeTypeTrader>
#include <KMimeType>
#include <KCharMacroExpander>

#include "klippersettings.h"
#include "clipcommandprocess.h"

// TODO: script-interface?
#include "history.h"
#include "historystringitem.h"

#if HAVE_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <fixx11h.h>
#endif

URLGrabber::URLGrabber(History* history):
    m_myCurrentAction(0L),
    m_myMenu(0L),
    m_myPopupKillTimer(new QTimer( this )),
    m_myPopupKillTimeout(8),
    m_stripWhiteSpace(true),
    m_history(history)
{
    m_myPopupKillTimer->setSingleShot( true );
    connect( m_myPopupKillTimer, SIGNAL(timeout()),
             SLOT(slotKillPopupMenu()));

    // testing
    /*
    ClipAction *action;
    action = new ClipAction( "^http:\\/\\/", "Web-URL" );
    action->addCommand("kfmclient exec %s", "Open with Konqi", true);
    action->addCommand("netscape -no-about-splash -remote \"openURL(%s, new-window)\"", "Open with Netscape", true);
    m_myActions->append( action );

    action = new ClipAction( "^mailto:", "Mail-URL" );
    action->addCommand("kmail --composer %s", "Launch kmail", true);
    m_myActions->append( action );

    action = new ClipAction( "^\\/.+\\.jpg$", "Jpeg-Image" );
    action->addCommand("kuickshow %s", "Launch KuickShow", true);
    action->addCommand("kview %s", "Launch KView", true);
    m_myActions->append( action );
    */
}


URLGrabber::~URLGrabber()
{
    qDeleteAll(m_myActions);
    m_myActions.clear();
    delete m_myMenu;
}

//
// Called from Klipper::slotRepeatAction, i.e. by pressing Ctrl-Alt-R
// shortcut. I.e. never from clipboard monitoring
//
void URLGrabber::invokeAction( const HistoryItem* item )
{
    m_myClipItem = item;
    actionMenu( item, false );
}


void URLGrabber::setActionList( const ActionList& list )
{
    qDeleteAll(m_myActions);
    m_myActions.clear();
    m_myActions = list;
}

void URLGrabber::matchingMimeActions(const QString& clipData)
{
    QUrl url(clipData);
    KConfigGroup cg(KSharedConfig::openConfig(), "Actions");
    if(!cg.readEntry("EnableMagicMimeActions",true)) {
        //kDebug() << "skipping mime magic due to configuration";
        return;
    }
    if(!url.isValid()) {
        //kDebug() << "skipping mime magic due to invalid url";
        return;
    }
    if(url.isRelative()) {  //openinng a relative path will just not work. what path should be used?
        //kDebug() << "skipping mime magic due to relative url";
        return;
    }
    if(url.isLocalFile()) {
        if ( clipData == "//") {
            //kDebug() << "skipping mime magic due to C++ comment //";
            return;
        }
        if(!QFile::exists(url.toLocalFile())) {
            //kDebug() << "skipping mime magic due to nonexistent localfile";
            return;
        }
    }

    // try to figure out if clipData contains a filename
    KMimeType::Ptr mimetype = KMimeType::findByUrl( url, 0,
                                                    false,
                                                    true /*fast mode*/ );

    // let's see if we found some reasonable mimetype.
    // If we do we'll populate menu with actions for apps
    // that can handle that mimetype

    // first: if clipboard contents starts with http, let's assume it's "text/html".
    // That is even if we've url like "http://www.kde.org/somescript.pl", we'll
    // still treat that as html page, because determining a mimetype using kio
    // might take a long time, and i want this function to be quick!
    if ( ( clipData.startsWith( QLatin1String("http://") ) || clipData.startsWith( QLatin1String("https://") ) )
         && mimetype->name() != "text/html" )
    {
        // use a fake path to create a mimetype that corresponds to "text/html"
        mimetype = KMimeType::findByPath( "/tmp/klipper.html", 0, true /*fast mode*/ );
    }

    if ( !mimetype->isDefault() ) {
        ClipAction* action = new ClipAction( QString(), mimetype->comment() );
        KService::List lst = KMimeTypeTrader::self()->query( mimetype->name(), "Application" );
        foreach( const KService::Ptr &service, lst ) {
            QHash<QChar,QString> map;
            map.insert( 'i', "--icon " + service->icon() );
            map.insert( 'c', service->name() );

            QString exec = service->exec();
            exec = KMacroExpander::expandMacros( exec, map ).trimmed();

            action->addCommand( ClipCommand( exec, service->name(), true, service->icon() ) );
        }
        if ( !lst.isEmpty() )
            m_myMatches.append( action );
    }
}

const ActionList& URLGrabber::matchingActions( const QString& clipData, bool automatically_invoked )
{
    m_myMatches.clear();

    matchingMimeActions(clipData);


    // now look for matches in custom user actions
    foreach (ClipAction* action, m_myActions) {
        if ( action->matches( clipData ) && (action->automatic() || !automatically_invoked) ) {
            m_myMatches.append( action );
        }
    }

    return m_myMatches;
}


void URLGrabber::checkNewData( const HistoryItem* item )
{
    // kDebug() << "** checking new data: " << clipData;
    actionMenu( item, true ); // also creates m_myMatches
}


void URLGrabber::actionMenu( const HistoryItem* item, bool automatically_invoked )
{
    if (!item) {
      qWarning("Attempt to invoke URLGrabber without an item");
      return;
    }
    QString text(item->text());
    if (m_stripWhiteSpace) {
        text = text.trimmed();
    }
    ActionList matchingActionsList = matchingActions( text, automatically_invoked );

    if (!matchingActionsList.isEmpty()) {
        // don't react on blacklisted (e.g. konqi's/netscape's urls) unless the user explicitly asked for it
        if ( automatically_invoked && isAvoidedWindow() ) {
            return;
        }

        m_myCommandMapper.clear();

        m_myPopupKillTimer->stop();

        m_myMenu = new KMenu;

        connect(m_myMenu, SIGNAL(triggered(QAction*)), SLOT(slotItemSelected(QAction*)));

        foreach (ClipAction* clipAct, matchingActionsList) {
            m_myMenu->addTitle(QIcon::fromTheme( "klipper" ),
                               i18n("%1 - Actions For: %2", clipAct->description(), KStringHandler::csqueeze(text, 45)));
            QList<ClipCommand> cmdList = clipAct->commands();
            int listSize = cmdList.count();
            for (int i=0; i<listSize;++i) {
                ClipCommand command = cmdList.at(i);

                QString item = command.description;
                if ( item.isEmpty() )
                    item = command.command;

                QString id = QUuid::createUuid().toString();
                QAction * action = new QAction(this);
                action->setData(id);
                action->setText(item);

                if (!command.icon.isEmpty())
                    action->setIcon(QIcon::fromTheme(command.icon));

                m_myCommandMapper.insert(id, qMakePair(clipAct,i));
                m_myMenu->addAction(action);
            }
        }

        // only insert this when invoked via clipboard monitoring, not from an
        // explicit Ctrl-Alt-R
        if ( automatically_invoked )
        {
            m_myMenu->addSeparator();
            QAction *disableAction = new QAction(i18n("Disable This Popup"), this);
            connect(disableAction, SIGNAL(triggered()), SIGNAL(sigDisablePopup()));
            m_myMenu->addAction(disableAction);
        }
        m_myMenu->addSeparator();

        QAction *cancelAction = new QAction(QIcon::fromTheme("dialog-cancel"), i18n("&Cancel"), this);
        connect(cancelAction, SIGNAL(triggered()), m_myMenu, SLOT(hide()));
        m_myMenu->addAction(cancelAction);
        m_myClipItem = item;

        if ( m_myPopupKillTimeout > 0 )
            m_myPopupKillTimer->start( 1000 * m_myPopupKillTimeout );

        emit sigPopup( m_myMenu );
    }
}


void URLGrabber::slotItemSelected(QAction* action)
{
    if (m_myMenu)
        m_myMenu->hide(); // deleted by the timer or the next action

    QString id = action->data().toString();

    if (id.isEmpty()) {
        kDebug() << "Klipper: no command associated";
        return;
    }

    // first is action ptr, second is command index
    QPair<ClipAction*, int> actionCommand = m_myCommandMapper.value(id);

    if (actionCommand.first)
        execute(actionCommand.first, actionCommand.second);
    else
        kDebug() << "Klipper: cannot find associated action";
}


void URLGrabber::execute( const ClipAction* action, int cmdIdx ) const
{
    if (!action) {
        kDebug() << "Action object is null";
        return;
    }

    ClipCommand command = action->command(cmdIdx);

    if ( command.isEnabled ) {
        QString text(m_myClipItem->text());
        if (m_stripWhiteSpace) {
            text = text.trimmed();
        }
        ClipCommandProcess* proc = new ClipCommandProcess(*action, command, text, m_history, m_myClipItem);
        if (proc->program().isEmpty()) {
            delete proc;
            proc = 0L;
        } else {
            proc->start();
        }
    }
}

void URLGrabber::loadSettings()
{
    m_stripWhiteSpace = KlipperSettings::stripWhiteSpace();
    m_myAvoidWindows = KlipperSettings::noActionsForWM_CLASS();
    m_myPopupKillTimeout = KlipperSettings::timeoutForActionPopups();

    qDeleteAll(m_myActions);
    m_myActions.clear();

    KConfigGroup cg(KSharedConfig::openConfig(), "General");
    int num = cg.readEntry("Number of Actions", 0);
    QString group;
    for ( int i = 0; i < num; i++ ) {
        group = QString("Action_%1").arg( i );
        m_myActions.append( new ClipAction( KSharedConfig::openConfig(), group ) );
    }
}

void URLGrabber::saveSettings() const
{
    KConfigGroup cg(KSharedConfig::openConfig(), "General");
    cg.writeEntry( "Number of Actions", m_myActions.count() );

    int i = 0;
    QString group;
    foreach (ClipAction* action, m_myActions) {
        group = QString("Action_%1").arg( i );
        action->save( KSharedConfig::openConfig(), group );
        ++i;
    }

    KlipperSettings::setNoActionsForWM_CLASS(m_myAvoidWindows);
}

// find out whether the active window's WM_CLASS is in our avoid-list
// digged a little bit in netwm.cpp
bool URLGrabber::isAvoidedWindow() const
{
#if HAVE_X11
    Display *d = QX11Info::display();
    static Atom wm_class = XInternAtom( d, "WM_CLASS", true );
    static Atom active_window = XInternAtom( d, "_NET_ACTIVE_WINDOW", true );
    Atom type_ret;
    int format_ret;
    unsigned long nitems_ret, unused;
    unsigned char *data_ret;
    long BUFSIZE = 2048;
    bool ret = false;
    Window active = 0L;
    QString wmClass;

    // get the active window
    if (XGetWindowProperty(d, DefaultRootWindow( d ), active_window, 0l, 1l,
                           False, XA_WINDOW, &type_ret, &format_ret,
                           &nitems_ret, &unused, &data_ret)
        == Success) {
        if (type_ret == XA_WINDOW && format_ret == 32 && nitems_ret == 1) {
            active = *((Window *) data_ret);
        }
        XFree(data_ret);
    }
    if ( !active )
        return false;

    // get the class of the active window
    if ( XGetWindowProperty(d, active, wm_class, 0L, BUFSIZE, False, XA_STRING,
                            &type_ret, &format_ret, &nitems_ret,
                            &unused, &data_ret ) == Success) {
        if ( type_ret == XA_STRING && format_ret == 8 && nitems_ret > 0 ) {
            wmClass = QString::fromUtf8( (const char *) data_ret );
            ret = (m_myAvoidWindows.indexOf( wmClass ) != -1);
        }

        XFree( data_ret );
    }

    return ret;
#else
    return false;
#endif
}


void URLGrabber::slotKillPopupMenu()
{
    if ( m_myMenu && m_myMenu->isVisible() )
    {
        if ( m_myMenu->geometry().contains( QCursor::pos() ) &&
             m_myPopupKillTimeout > 0 )
        {
            m_myPopupKillTimer->start( 1000 * m_myPopupKillTimeout );
            return;
        }
    }

    if ( m_myMenu ) {
        m_myMenu->deleteLater();
        m_myMenu = 0;
    }
}

///////////////////////////////////////////////////////////////////////////
////////

ClipCommand::ClipCommand(const QString&_command, const QString& _description,
                         bool _isEnabled, const QString& _icon, Output _output)
    : command(_command),
      description(_description),
      isEnabled(_isEnabled),
      output(_output)
{

    if (!_icon.isEmpty())
        icon = _icon;
    else
    {
        // try to find suitable icon
        QString appName = command.section( ' ', 0, 0 );
        if ( !appName.isEmpty() )
        {
            QPixmap iconPix = KIconLoader::global()->loadIcon(
                                         appName, KIconLoader::Small, 0,
                                         KIconLoader::DefaultState,
                                         QStringList(), 0, true /* canReturnNull */ );
            if ( !iconPix.isNull() )
                icon = appName;
            else
                icon.clear();
        }
    }
}


ClipAction::ClipAction( const QString& regExp, const QString& description, bool automatic )
    : m_myRegExp( regExp ), m_myDescription( description ), m_automatic(automatic)
{
}

ClipAction::ClipAction( KSharedConfigPtr kc, const QString& group )
    : m_myRegExp( kc->group(group).readEntry("Regexp") ),
      m_myDescription (kc->group(group).readEntry("Description") ),
      m_automatic(kc->group(group).readEntry("Automatic", QVariant(true)).toBool() )
{
    KConfigGroup cg(kc, group);

    int num = cg.readEntry( "Number of commands", 0 );

    // read the commands
    for ( int i = 0; i < num; i++ ) {
        QString _group = group + "/Command_%1";
        KConfigGroup _cg(kc, _group.arg(i));

        addCommand( ClipCommand(_cg.readPathEntry( "Commandline", QString() ),
                                _cg.readEntry( "Description" ), // i18n'ed
                                _cg.readEntry( "Enabled" , false),
                                _cg.readEntry( "Icon"),
                                static_cast<ClipCommand::Output>(_cg.readEntry( "Output", QVariant(ClipCommand::IGNORE)).toInt())));
    }
}


ClipAction::~ClipAction()
{
    m_myCommands.clear();
}


void ClipAction::addCommand( const ClipCommand& cmd )
{
    if ( cmd.command.isEmpty() )
        return;

    m_myCommands.append( cmd );
}

void ClipAction::replaceCommand( int idx, const ClipCommand& cmd )
{
    if ( idx < 0 || idx >= m_myCommands.count() ) {
        kDebug() << "wrong command index given";
        return;
    }

    m_myCommands.replace(idx, cmd);
}


// precondition: we're in the correct action's group of the KConfig object
void ClipAction::save( KSharedConfigPtr kc, const QString& group ) const
{
    KConfigGroup cg(kc, group);
    cg.writeEntry( "Description", description() );
    cg.writeEntry( "Regexp", regExp() );
    cg.writeEntry( "Number of commands", m_myCommands.count() );
    cg.writeEntry( "Automatic", automatic() );

    int i=0;
    // now iterate over all commands of this action
    foreach (const ClipCommand& cmd, m_myCommands) {
        QString _group = group + "/Command_%1";
        KConfigGroup cg(kc, _group.arg(i));

        cg.writePathEntry( "Commandline", cmd.command );
        cg.writeEntry( "Description", cmd.description );
        cg.writeEntry( "Enabled", cmd.isEnabled );
        cg.writeEntry( "Icon", cmd.icon );
        cg.writeEntry( "Output", static_cast<int>(cmd.output) );

        ++i;
    }
}

#include "urlgrabber.moc"
