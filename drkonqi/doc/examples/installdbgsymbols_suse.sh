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
    konsole -e sh -c "echo \$\$ > $fifo; $1; exit_status=\$?; sleep 1; rm $fifo; echo \$exit_status > $fifo" &

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

# check misc script dependencies
check_dep()
{
    which $1 >/dev/null
    if [ "$?" != "0" ]; then
        # check for availability of kdialog
        which kdialog >/dev/null
        if [ "$?" != "0" ]; then
            xmessage -center "$1 was not found on your system. Please install $1 and try again."
        else
            kdialog --sorry "$1 was not found on your system. Please install $1 and try again."
        fi
        exit 1
    fi
}

check_dep pbuildid
check_dep konsole

# start searching for packages
packages=""
while [ "$1" != "" ];
do
    # look for the build id
    file -L "$1" | grep ELF >/dev/null
    if [ "$?" = "0" ]; then
        buildid=`pbuildid "$1" | cut -d ' ' -f 2`
        package="\"debuginfo(build-id)=$buildid\""
        packages="$packages $package"
    fi

    shift
done

run_in_terminal "sudo zypper install -C $packages"

if [ "$?" = "1" ]; then
    exit 3
elif [ "$exit_status" = "0" ]; then
    exit 0
else
    exit 1
fi
