#!/bin/sh
#
# Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


# This function runs a command in a terminal
# The first argument ($1) must be the full command to run.
# It returns 0 if the command finished or 1 if the user closed
# the terminal without waiting for the command to finish.
# The return value of the command is saved in the $exit_status variable.
run_in_terminal()
{
    local fifo=/tmp/drkonqi-fifo-$$
    mkfifo $fifo

    # start terminal
    x-terminal-emulator -e sh -c "echo \$\$ > $fifo; $1; exit_status=\$?; sleep 1; rm $fifo; echo \$exit_status > $fifo" &

    # wait for it to finish
    local pid=`cat $fifo`
    while [ "$?" = "0" ]; do
        sleep 1
        kill -0 $pid 2>/dev/null
    done

    # check if terminal has finished succesfully and return the command's exit status
    local canceled=0
    if [ -p $fifo ]; then
        # terminal was closed before finishing execution
        canceled=1
    else
        exit_status=`cat $fifo`
        #echo "\"$1\" returned: $exit_status"
    fi
    rm $fifo
    return $canceled
}

# check for availability of kdialog
which kdialog >/dev/null
if [ "$?" != "0" ]; then
    xmessage -center "Could not find kdialog (part of kdebase). Please install kdialog and try again."
    exit 1
fi

# check misc script dependencies
check_dep()
{
    which $1 >/dev/null
    if [ "$?" != "0" ]; then
        kdialog --sorry "$1 was not found on your system. Please install $1 and try again."
        exit 1
    fi
}

check_dep apt-file
check_dep qdbus

# update apt-file database
run_in_terminal "apt-file update"

if [ "$?" = "1" ]; then
    exit 3
elif [ "$exit_status" != "0" ]; then
    kdialog --sorry "apt-file failed to update package lists."
    exit 1
fi

# start searching for packages
packages=""
progress_counter=0
dbus_handle=`kdialog --progressbar "Searching for packages that contain the requested debug symbols..." $#`

while [ "$1" != "" ];
do
    # dereference symlinks
    cur_file=$1
    while [ -L "$cur_file" ]; do
        cur_file="`dirname $cur_file`/`ls -l $cur_file | cut -d ' ' -f 10`"
    done

    # look for the package
    expr match "$cur_file" ".*libQt.*" >/dev/null
    if [ "$?" = "0" ]; then
        # HACK for Qt, which doesn't install debug symbols in /usr/lib/debug like everybody else
        package="libqt4-dbg"
    else
        package=`apt-file search --non-interactive --package-only --fixed-string "/usr/lib/debug$cur_file"`
    fi
    packages="$packages $package"

    # update progress dialog
    progress_counter=$(($progress_counter+1))
    qdbus $dbus_handle Set org.kde.kdialog.ProgressDialog value $progress_counter

    # check if dialog was closed
    if [ "$?" != "0" ]; then
        exit 3
    fi

    shift
done

# filter out duplicates
packages=`echo "$packages" | tr " " "\n" | sort | uniq | tr "\n" " "`

# close the progress dialog
qdbus $dbus_handle close

# if there are no packages to install, exit
trimmed_packages=`echo $packages | tr -d "[:blank:]"`
if [ -z "$trimmed_packages" ]; then
    exit 2 # note that we don't need to display an error message here. drkonqi will do it for us.
fi

kdialog --yesno "You need to install the following packages: $packages
Would you like drkonqi to attempt to install them now?"

if [ "$?" = "0" ]; then
    # determine package manager
    package_manager=aptitude
    which $package_manager >/dev/null
    if [ "$?" != "0" ]; then
        package_manager=apt-get
    fi

    run_in_terminal "su-to-root -c '$package_manager install $packages'"

    if [ "$?" = "1" ]; then
        exit 3
    elif [ "$exit_status" = "0" ]; then
        exit 0
    else
        exit 1
    fi
else
    exit 3
fi
