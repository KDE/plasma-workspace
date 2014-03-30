/****************************************************************************

 Copyright (C) 2005 Lubos Lunak        <l.lunak@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

****************************************************************************/

#undef QT_NO_CAST_ASCII

// See description in kstartupconfig.cpp .
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <kcomponentdata.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kshell.h>
#include <KGlobal>


#if defined _WIN32 || defined _WIN64
#define KPATH_SEPARATOR ';'
#else
#define KPATH_SEPARATOR ':'
#endif

static QString get_entry( QString* ll )
    {
    QString& l = *ll;
    l = l.trimmed();
    if( l.isEmpty())
        return QString();
    QString ret;
    if( l[ 0 ] == '\'' )
        {
        int pos = 1;
        while( pos < l.length() && l[ pos ] != '\'' )
            ret += l[ pos++ ];
        if( pos >= l.length())
            {
            *ll = QString();
            return QString();
            }
        *ll = l.mid( pos + 1 );
        return ret;
        }
    int pos = 0;
    while( pos < l.length() && l[ pos ] != ' ' )
        ret += l[ pos++ ];
    *ll = l.mid( pos );
    return ret;
    }

int main( int argc, char **argv )
    {

    QString keysname = KStandardDirs::locateLocal( "config", "startupconfigkeys" );
    QFile keys( keysname );
    if( !keys.open( QIODevice::ReadOnly ))
        return 3;
    QFile f1( KStandardDirs::locateLocal( "config", "startupconfig" ));
    if( !f1.open( QIODevice::WriteOnly ))
        return 4;
    QFile f2( KStandardDirs::locateLocal( "config", "startupconfigfiles" ));
    if( !f2.open( QIODevice::WriteOnly ))
        return 5;
    QTextStream startupconfig( &f1 );
    QTextStream startupconfigfiles( &f2 );
    startupconfig << "#! /bin/sh\n";
    for(;;)
        {
	  QString line;
	  {
	    QByteArray buf;
	    buf.resize(1024);
	    if( keys.readLine( buf.data(), buf.length() ) < 0 )
	      break;
	    line = QString::fromLocal8Bit(buf);
	  }
        line = line.trimmed();
        if( line.isEmpty())
            break;
        QString tmp = line;
        QString file, group, key, def;
        file = get_entry( &tmp );
        group = get_entry( &tmp );
        key = get_entry( &tmp );
        def = get_entry( &tmp );
        if( file.isEmpty() || group.isEmpty())
            return 6;
        if( group.startsWith( '[' ) && group.endsWith( ']' ) )
            { // whole config group
            KConfig cfg( file );
            group = group.mid( 1, group.length() - 2 );
            KConfigGroup cg(&cfg, group);
            QMap< QString, QString > entries = cg.entryMap( );
            startupconfig << "# " << line << "\n";
            for( QMap< QString, QString >::ConstIterator it = entries.constBegin();
                 it != entries.constEnd();
                 ++it )
                {
                QString key = it.key();
                QString value = *it;
                startupconfig << file.replace( ' ', '_' ).toLower()
                    << "_" << group.replace( ' ', '_' ).toLower()
                    << "_" << key.replace( ' ', '_' ).toLower()
                    << "=" << KShell::quoteArg( value ) << "\n";
                }
            }
        else
            { // a single key
            if( key.isEmpty())
                return 7;
            KConfig cfg( file );
            KConfigGroup cg(&cfg, group );
            QString value = cg.readEntry( key, def );
            startupconfig << "# " << line << "\n";
            startupconfig << file.replace( ' ', '_' ).toLower()
                << "_" << group.replace( ' ', '_' ).toLower()
                << "_" << key.replace( ' ', '_' ).toLower()
                << "=" << KShell::quoteArg( value ) << "\n";
            }
        startupconfigfiles << line << endl;
        // use even currently non-existing paths in $KDEDIRS
        const QStringList dirs = KGlobal::dirs()->kfsstnd_prefixes().split( KPATH_SEPARATOR, QString::SkipEmptyParts);
        for( QStringList::ConstIterator it = dirs.constBegin();
             it != dirs.constEnd();
             ++it )
            {
            QString cfg = *it + "share/config/" + file;
            if( KStandardDirs::exists( cfg ))
                startupconfigfiles << cfg << "\n";
            else
                startupconfigfiles << "!" << cfg << "\n";
            }
        startupconfigfiles << "*\n";
        }

        // Get languages by priority from KLocale.
        const QStringList langs = KGlobal::locale()->languageList();
        startupconfig << "klocale_languages=" << langs.join( ":" ) << "\n";
    return 0;
    }
