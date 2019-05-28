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
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>

#include <KConfig>
#include <KConfigGroup>
#include <KShell>

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
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QDir().mkdir(path);

    QString keysname = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("startupconfigkeys"));
    QFile keys( keysname );
    if( !keys.open( QIODevice::ReadOnly ))
        return 3;
    QFile f1(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/startupconfig"));
    if( !f1.open( QIODevice::WriteOnly ))
        return 4;
    QFile f2(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/startupconfigfiles"));
    if( !f2.open( QIODevice::WriteOnly ))
        return 5;
    QTextStream startupconfig( &f1 );
    QTextStream startupconfigfiles( &f2 );
    startupconfig << "#! /bin/sh\n";
    for(;;)
        {
        QString line = QString::fromLocal8Bit(keys.readLine()).trimmed();
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
                startupconfig << "export " << file.replace( ' ', '_' ).toLower()
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
            startupconfig << "export " << file.replace( ' ', '_' ).toLower()
                << "_" << group.replace( ' ', '_' ).toLower()
                << "_" << key.replace( ' ', '_' ).toLower()
                << "=" << KShell::quoteArg( value ) << "\n";
            }
        startupconfigfiles << line << endl;

        //we want a list of ~/.config + /usr/share/config (on a normal setup).
        //These files may not exist yet as the latter is used by kconf_update
        QStringList configDirs = QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation);
        foreach (const QString &location, QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
            //some people accidentally have empty prefixes in their XDG_DATA_DIRS, strip those out
            if (location.isEmpty()) {
                continue;
            }
            configDirs << (location + "/config");
        }

        foreach (const QString &configDir, configDirs) {
            const QString cfg = configDir + '/' + file;
            if (QFile::exists(cfg))
                startupconfigfiles << cfg << "\n";
            else
                startupconfigfiles << "!" << cfg << "\n";
        }

        startupconfigfiles << "*\n";
    }

    return 0;
}
