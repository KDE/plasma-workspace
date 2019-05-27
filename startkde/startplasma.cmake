#!/bin/sh
#
#  DEFAULT Plasma STARTUP SCRIPT ( @PROJECT_VERSION@ )
#


kdeinit5_shutdown

unset KDE_FULL_SESSION
xprop -root -remove KDE_FULL_SESSION
unset KDE_SESSION_VERSION
xprop -root -remove KDE_SESSION_VERSION
unset KDE_SESSION_UID

echo 'startplasma: Done.'  1>&2
