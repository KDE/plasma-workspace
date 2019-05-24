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

/*

This utility helps to have some configuration options available in startkde
without the need to launch anything linked to KDE libraries (which may need
some time to load).

The configuration options are written to $KDEHOME/share/config/startupconfigkeys,
one option per line, as <file> <group> <key> <default>. It is possible to
use ' for quoting multiword entries. Values of these options will be written
to $KDEHOME/share/config/startupconfig as a shell script that will set
the values to shell variables, named <file>_<group>_<key> (all spaces replaced
by underscores, everything lowercase). So e.g. line
"ksplashrc KSplash Theme Default" may result in "ksplashrc_ksplash_theme=Default".

In order to real a whole group it is possible to use <file> <[group]>, e.g.
"ksplashrc [KSplash]", which will set shell variables for all keys in the group.
It is not possible to specify default values, but since the configuration options
are processed in the order they are specified this can be solved by first
specifying a group and then all the entries that need default values.

When a kconf_update script is used to update such option, kstartupconfig is run
before kconf_update and therefore cannot see the change in time. To avoid this
problem, together with the kconf_update script also the matching global config
file should be updated (any change, kstartupconfig will see the timestamp change).

Note that the kdeglobals config file is not used as a depedendency for other config
files.

Since the checking is timestamp-based, config files that are frequently updated
should not be used.

Kstartupconfig works by storing every line from startupconfigkeys in file startupconfigfiles
followed by paths of all files that are relevant to the option. Non-existent files
have '!' prepended (for the case they'll be later created), the list of files is
terminated by line containing '*'. If the timestamps of all relevant files are older
than the timestamp of the startupconfigfile file, there's no need to update anything.
Otherwise kdostartupconfig is launched to create or update all the necessary files
(which already requires loading KDE libraries, but this case should be rare).

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>

int kStartupConfig()
{
    time_t config_time;
    FILE* config;
    FILE* keys;
    struct stat st;
    std::string kdehome;
    std::string filename;

    if (getenv( "XDG_CONFIG_HOME" )) {
        kdehome = getenv("XDG_CONFIG_HOME");
    } else {
        kdehome = std::string(getenv("HOME")) + "/.config";
    }
    filename = kdehome + "/startupconfig";
    if (access(filename.c_str(), R_OK) != 0)
        goto doit;
    filename = kdehome + "/startupconfigfiles";
    if (stat(filename.c_str(), &st) != 0)
        goto doit;
    config_time = st.st_mtime;
    config = fopen(filename.c_str(), "r");
    if( config == nullptr )
        goto doit;
    filename = kdehome + "/startupconfigkeys";
    keys = fopen(filename.c_str(), "r");
    if( keys == nullptr )
    {
        fclose(config);
        return 2;
    }
    for(;;)
        {
        char* nl;
        char keyline[ 1024 ];
        char line[ 1024 ];

        if( fgets( keyline, 1023, keys ) == nullptr )
            return 0;
        if( (nl = strchr( keyline, '\n' )) )
            *nl = '\0';
        if( fgets( line, 1023, config ) == nullptr )
            break;
        if( (nl = strchr( line, '\n' )) )
            *nl = '\0';
        if( strcmp( keyline, line ) != 0 )
            break;
        for(;;)
            {
            if( fgets( line, 1023, config ) == nullptr )
                goto doit2;
            if( (nl = strchr( line, '\n' )) )
                *nl = '\0';
            if( *line == '\0' )
                goto doit2;
            if( *line == '*' )
                break;
            if( *line == '!' )
                {
                if( access( line + 1, R_OK ) == 0 )
                    goto doit2; /* file now exists -> update */
                }
            else
                {
                if( stat( line, &st ) != 0 )
                    goto doit2;
                if( st.st_mtime > config_time )
                    goto doit2;
                }
            }
        }
  doit2:
    fclose( keys );
    fclose( config );
  doit:
    return system( "kdostartupconfig5" );
    }
