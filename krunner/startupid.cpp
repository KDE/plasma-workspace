/* This file is part of the KDE project
   Copyright (C) 2001 Lubos Lunak <l.lunak@kde.org>
   Copyright (C) 2010 Martin Gräßlin <kde@martin-graesslin.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "startupid.h"

#include <config-X11.h>

#include "klaunchsettings.h"

#include <kiconloader.h>
#include <kmanagerselection.h>
#include <QCursor>
#include <kapplication.h>
#include <QImage>
#include <QBitmap>
//Added by qt3to4:
#include <QPixmap>
#include <QPainter>
#include <kconfig.h>
#include <X11/Xlib.h>
#include <QX11Info>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#define KDE_STARTUP_ICON QLatin1String( "kmenu" )

#ifdef HAVE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif

enum kde_startup_status_enum { StartupPre, StartupIn, StartupDone };
static kde_startup_status_enum kde_startup_status = StartupPre;
static Atom kde_splash_progress;

StartupId::StartupId( QWidget* parent, const char* name )
    :   QWidget( parent ),
        startup_info( KStartupInfo::CleanOnCantDetect ),
        startup_window( None ),
        blinking( true ),
        bouncing( false ),
        selection_watcher( new KSelectionWatcher( "_KDE_STARTUP_FEEDBACK", -1, this ))
    {
        setObjectName( QLatin1String( name ) );
    hide(); // is QWidget only because of x11Event()
    if( kde_startup_status == StartupPre )
        {
        kde_splash_progress = XInternAtom( QX11Info::display(), "_KDE_SPLASH_PROGRESS", False );
        XWindowAttributes attrs;
        XGetWindowAttributes( QX11Info::display(), QX11Info::appRootWindow(), &attrs);
        XSelectInput( QX11Info::display(), QX11Info::appRootWindow(), attrs.your_event_mask | SubstructureNotifyMask);
        kapp->installX11EventFilter( this );
        }
    update_timer.setSingleShot( true );
    connect( &update_timer, SIGNAL(timeout()), SLOT(update_startupid()));
    connect( &startup_info,
        SIGNAL(gotNewStartup(KStartupInfoId,KStartupInfoData)),
        SLOT(gotNewStartup(KStartupInfoId,KStartupInfoData)));
    connect( &startup_info,
        SIGNAL(gotStartupChange(KStartupInfoId,KStartupInfoData)),
        SLOT(gotStartupChange(KStartupInfoId,KStartupInfoData)));
    connect( &startup_info,
        SIGNAL(gotRemoveStartup(KStartupInfoId,KStartupInfoData)),
        SLOT(gotRemoveStartup(KStartupInfoId)));
    connect( selection_watcher, SIGNAL(newOwner(Window)), SLOT(newOwner()));
    connect( selection_watcher, SIGNAL(lostOwner()), SLOT(lostOwner()));
    active_selection = ( selection_watcher->owner() != None );
    }

StartupId::~StartupId()
    {
    stop_startupid();
    }

void StartupId::configure()
    {
    startup_info.setTimeout( KLaunchSettings::timeout());
    blinking = KLaunchSettings::blinking();
    bouncing = KLaunchSettings::bouncing();
    }

void StartupId::gotNewStartup( const KStartupInfoId& id_P, const KStartupInfoData& data_P )
    {
    if( active_selection )
        return;
    QString icon = data_P.findIcon();
    current_startup = id_P;
    startups[ id_P ] = icon;
    start_startupid( icon );
    }

void StartupId::gotStartupChange( const KStartupInfoId& id_P, const KStartupInfoData& data_P )
    {
    if( active_selection )
        return;
    if( current_startup == id_P )
        {
        QString icon = data_P.findIcon();
        if( !icon.isEmpty() && icon != startups[ current_startup ] )
            {
            startups[ id_P ] = icon;
            start_startupid( icon );
            }
        }
    }

void StartupId::gotRemoveStartup( const KStartupInfoId& id_P )
    {
    if( active_selection )
        return;
    startups.remove( id_P );
    if( startups.count() == 0 )
        {
        current_startup = KStartupInfoId(); // null
        if( kde_startup_status == StartupIn )
            start_startupid( KDE_STARTUP_ICON );
        else
            stop_startupid();
        return;
        }
    current_startup = startups.begin().key();
    start_startupid( startups[ current_startup ] );
    }

bool StartupId::x11Event( XEvent* e )
    {
    if( e->type == ClientMessage && e->xclient.window == QX11Info::appRootWindow()
        && e->xclient.message_type == kde_splash_progress )
        {
        const char* s = e->xclient.data.b;
        if( strcmp( s, "desktop" ) == 0 && kde_startup_status == StartupPre )
            {
            kde_startup_status = StartupIn;
            if( startups.count() == 0 )
                start_startupid( KDE_STARTUP_ICON );
            // 60(?) sec timeout - shouldn't be hopefully needed anyway, ksmserver should have it too
            QTimer::singleShot( 60000, this, SLOT(finishKDEStartup()));
            }
        else if( strcmp( s, "ready" ) == 0 && kde_startup_status < StartupDone )
            QTimer::singleShot( 2000, this, SLOT(finishKDEStartup()));
        }
    return false;
    }

void StartupId::finishKDEStartup()
    {
    kde_startup_status = StartupDone;
    kapp->removeX11EventFilter( this );
    if( startups.count() == 0 )
        stop_startupid();
    }

void StartupId::stop_startupid()
    {
    if( startup_window != None )
        XDestroyWindow( QX11Info::display(), startup_window );
    startup_window = None;
    if( blinking )
        for( int i = 0;
             i < NUM_BLINKING_PIXMAPS;
             ++i )
            pixmaps[ i ] = QPixmap(); // null
    update_timer.stop();
    }

static QPixmap scalePixmap( const QPixmap& pm, int w, int h )
{
    QImage scaled = pm.toImage().scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (scaled.format() != QImage::Format_ARGB32_Premultiplied && scaled.format() != QImage::Format_ARGB32)
        scaled = scaled.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    QImage result(20, 20, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&result);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(result.rect(), Qt::transparent);
    p.drawImage((20 - w) / 2, (20 - h) / 2, scaled, 0, 0, w, h);
    return QPixmap::fromImage(result);
}

// Transparent images are converted to 32bpp pixmaps, but
// setting those as window background needs ARGB visual
// and compositing - convert to 24bpp, at least for now.
static QPixmap make24bpp( const QPixmap& p )
    {
    QPixmap ret( p.size());
    QPainter pt( &ret );
    pt.drawPixmap( 0, 0, p );
    pt.end();
    ret.setMask( p.mask());
    return ret;
    }

void StartupId::start_startupid( const QString& icon_P )
    {

    const QColor startup_colors[ StartupId::NUM_BLINKING_PIXMAPS ]
    = { Qt::black, Qt::darkGray, Qt::lightGray, Qt::white, Qt::white };


    QPixmap icon_pixmap = KIconLoader::global()->loadIcon( icon_P, KIconLoader::Small, 0,
        KIconLoader::DefaultState, QStringList(), 0, true ); // return null pixmap if not found
    if( icon_pixmap.isNull())
        icon_pixmap = SmallIcon( QLatin1String( "system-run" ) );
    if( startup_window == None )
        {
        XSetWindowAttributes attrs;
        attrs.override_redirect = True;
        attrs.save_under = True; // useful saveunder if possible to avoid redrawing
        attrs.colormap = QX11Info::appColormap();
        attrs.background_pixel = WhitePixel( QX11Info::display(), QX11Info::appScreen());
        attrs.border_pixel = BlackPixel( QX11Info::display(), QX11Info::appScreen());
        startup_window = XCreateWindow( QX11Info::display(), DefaultRootWindow( QX11Info::display()),
            0, 0, 1, 1, 0, QX11Info::appDepth(), InputOutput, static_cast< Visual* >( QX11Info::appVisual()),
            CWOverrideRedirect | CWSaveUnder | CWColormap | CWBackPixel | CWBorderPixel, &attrs );
        XClassHint class_hint;
        QByteArray cls = qAppName().toLatin1();
        class_hint.res_name = cls.data();
        class_hint.res_class = const_cast< char* >( QX11Info::appClass());
        XSetWMProperties( QX11Info::display(), startup_window, NULL, NULL, NULL, 0, NULL, NULL, &class_hint );
        XChangeProperty( QX11Info::display(), winId(),
            XInternAtom( QX11Info::display(), "WM_WINDOW_ROLE", False ), XA_STRING, 8, PropModeReplace,
            (unsigned char *)"startupfeedback", strlen( "startupfeedback" ));
        }
    XResizeWindow( QX11Info::display(), startup_window, icon_pixmap.width(), icon_pixmap.height());
    if( blinking )
        { // no mask
        XShapeCombineMask( QX11Info::display(), startup_window, ShapeBounding, 0, 0, None, ShapeSet );
        int window_w = icon_pixmap.width();
        int window_h = icon_pixmap.height();
        for( int i = 0;
             i < NUM_BLINKING_PIXMAPS;
             ++i )
            {
            pixmaps[ i ] = QPixmap( window_w, window_h );
            pixmaps[ i ].fill( startup_colors[ i ] );
            QPainter p( &pixmaps[ i ] );
            p.drawPixmap( 0, 0, icon_pixmap );
            p.end();
            }
        color_index = 0;
        }
    else if( bouncing )
        {
        XResizeWindow( QX11Info::display(), startup_window, 20, 20 );
        pixmaps[ 0 ] = make24bpp( scalePixmap( icon_pixmap, 16, 16 ));
        pixmaps[ 1 ] = make24bpp( scalePixmap( icon_pixmap, 14, 18 ));
        pixmaps[ 2 ] = make24bpp( scalePixmap( icon_pixmap, 12, 20 ));
        pixmaps[ 3 ] = make24bpp( scalePixmap( icon_pixmap, 18, 14 ));
        pixmaps[ 4 ] = make24bpp( scalePixmap( icon_pixmap, 20, 12 ));
        frame = 0;
        }
    else
        {
        icon_pixmap = make24bpp( icon_pixmap );
        if( !icon_pixmap.mask().isNull() ) // set mask
            XShapeCombineMask( QX11Info::display(), startup_window, ShapeBounding, 0, 0,
                icon_pixmap.mask().handle(), ShapeSet );
        else // clear mask
            XShapeCombineMask( QX11Info::display(), startup_window, ShapeBounding, 0, 0, None, ShapeSet );
        XSetWindowBackgroundPixmap( QX11Info::display(), startup_window, icon_pixmap.handle());
        XClearWindow( QX11Info::display(), startup_window );
        }
    update_startupid();
    }

namespace
{
const int X_DIFF = 15;
const int Y_DIFF = 15;
const int color_to_pixmap[] = { 0, 1, 2, 3, 2, 1 };
const int frame_to_yoffset[] =
  {
    -5, -1, 2, 5, 8, 10, 12, 13, 15, 15, 15, 15, 14, 12, 10, 8, 5, 2, -1, -5
  };
const int frame_to_pixmap[] =
  {
    0, 0, 0, 1, 2, 2, 1, 0, 3, 4, 4, 3, 0, 1, 2, 2, 1, 0, 0, 0
  };
}

void StartupId::update_startupid()
    {
    int yoffset = 0;
    if( blinking )
        {
        XSetWindowBackgroundPixmap( QX11Info::display(), startup_window,
            pixmaps[ color_to_pixmap[ color_index ]].handle());
        XClearWindow( QX11Info::display(), startup_window );
        if( ++color_index >= ( sizeof( color_to_pixmap ) / sizeof( color_to_pixmap[ 0 ] )))
            color_index = 0;
        }
    else if( bouncing )
        {
        yoffset = frame_to_yoffset[ frame ];
        QPixmap pixmap = pixmaps[ frame_to_pixmap[ frame ] ];
        XSetWindowBackgroundPixmap( QX11Info::display(), startup_window, pixmap.handle());
        XClearWindow( QX11Info::display(), startup_window );
        if ( !pixmap.mask().isNull() ) // set mask
            XShapeCombineMask( QX11Info::display(), startup_window, ShapeBounding, 0, 0,
                pixmap.mask().handle(), ShapeSet );
        else // clear mask
            XShapeCombineMask( QX11Info::display(), startup_window, ShapeBounding, 0, 0, None, ShapeSet );
        if ( ++frame >= ( sizeof( frame_to_yoffset ) / sizeof( frame_to_yoffset[ 0 ] ) ) )
            frame = 0;
        }
    Window dummy1, dummy2;
    int x, y;
    int dummy3, dummy4;
    unsigned int dummy5;
    if( !XQueryPointer( QX11Info::display(), QX11Info::appRootWindow(), &dummy1, &dummy2, &x, &y, &dummy3, &dummy4, &dummy5 ))
        {
        XUnmapWindow( QX11Info::display(), startup_window );
        update_timer.start( 100 );
        return;
        }
    QPoint c_pos( x, y );
    int cursor_size = 0;
#ifdef HAVE_XCURSOR
    cursor_size = XcursorGetDefaultSize( QX11Info::display());
#endif
    int X_DIFF;
    if( cursor_size <= 16 )
        X_DIFF = 8 + 7;
    else if( cursor_size <= 32 )
        X_DIFF = 16 + 7;
    else if( cursor_size <= 48 )
        X_DIFF = 24 + 7;
    else
        X_DIFF = 32 + 7;
    int Y_DIFF = X_DIFF;
    XMoveWindow( QX11Info::display(), startup_window, c_pos.x() + X_DIFF, c_pos.y() + Y_DIFF + yoffset );
    XMapWindow( QX11Info::display(), startup_window );
    XRaiseWindow( QX11Info::display(), startup_window );
    update_timer.start( bouncing ? 30 : 100 );
    QApplication::flush();
    }

void StartupId::newOwner()
    {
    active_selection = true;
    }

void StartupId::lostOwner()
    {
    active_selection = false;
    }

#include "startupid.moc"
