#!/bin/sh
#
#  DEFAULT Plasma STARTUP SCRIPT ( @PROJECT_VERSION@ )
#

# Boot sequence:
#
# kdeinit is used to fork off processes which improves memory usage
# and startup time.
#
# * kdeinit starts klauncher first.
# * Then kded is started. kded is responsible for keeping the sycoca
#   database up to date. When an up to date database is present it goes
#   into the background and the startup continues.
# * Then kdeinit starts kcminit. kcminit performs initialisation of
#   certain devices according to the user's settings
#
# * Then ksmserver is started which takes control of the rest of the startup sequence

# We need to create config folder so we can write startupconfigkeys
if [  ${XDG_CONFIG_HOME} ]; then
  configDir=$XDG_CONFIG_HOME;
else
  configDir=${HOME}/.config; #this is the default, http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
fi

[ -r $configDir/startupconfig ] && . $configDir/startupconfig

xrdb -quiet -merge -nocpp <<EOF
Xft.dpi: $QT_WAYLAND_FORCE_DPI
EOF

dl=$DESKTOP_LOCKED
unset DESKTOP_LOCKED # Don't want it in the environment

ksplash_pid=
if test -z "$dl"; then
  # the splashscreen and progress indicator
  case "$ksplashrc_ksplash_engine" in
    KSplashQML)
      ksplash_pid=`ksplashqml "${ksplashrc_ksplash_theme}" --pid`
      ;;
    None)
      ;;
    *)
      ;;
  esac
fi

#In wayland we want Plasma to use Qt's scaling
export PLASMA_USE_QT_SCALING=1

# Set a left cursor instead of the standard X11 "X" cursor, since I've heard
# from some users that they're confused and don't know what to do. This is
# especially necessary on slow machines, where starting KDE takes one or two
# minutes until anything appears on the screen.
#
# If the user has overwritten fonts, the cursor font may be different now
# so don't move this up.
#
xsetroot -cursor_name left_ptr

# Get Ghostscript to look into user's KDE fonts dir for additional Fontmap
usr_fdir=$HOME/.fonts
if test -n "$GS_LIB" ; then
    GS_LIB=$usr_fdir:$GS_LIB
    export GS_LIB
else
    GS_LIB=$usr_fdir
    export GS_LIB
fi

echo 'startplasma: Starting up...'  1>&2

# export our session variables to the Xwayland server
xprop -root -f KDE_FULL_SESSION 8t -set KDE_FULL_SESSION true
xprop -root -f KDE_SESSION_VERSION 32c -set KDE_SESSION_VERSION 5

# At this point all environment variables are set, let's send it to the DBus session server to update the activation environment
if which dbus-update-activation-environment >/dev/null 2>/dev/null ; then
    dbus-update-activation-environment --systemd --all
else
    @CMAKE_INSTALL_FULL_LIBEXECDIR@/ksyncdbusenv
fi
if test $? -ne 0; then
  # Startup error
  echo 'startplasma: Could not sync environment to dbus.'  1>&2
  exit 1
fi

# We set LD_BIND_NOW to increase the efficiency of kdeinit.
# kdeinit unsets this variable before loading applications.
LD_BIND_NOW=true @CMAKE_INSTALL_FULL_LIBEXECDIR_KF5@/start_kdeinit_wrapper --kded +kcminit_startup
if test $? -ne 0; then
  # Startup error
  echo 'startplasma: Could not start kdeinit5. Check your installation.'  1>&2
  test -n "$ksplash_pid" && kill "$ksplash_pid" 2>/dev/null
  xmessage -geometry 500x100 "Could not start kdeinit5. Check your installation."
  exit 1
fi

qdbus org.kde.KSplash /KSplash org.kde.KSplash.setStage kinit &

# finally, give the session control to the session manager
# see kdebase/ksmserver for the description of the rest of the startup sequence
# if the KDEWM environment variable has been set, then it will be used as KDE's
# window manager instead of kwin.
# if KDEWM is not set, ksmserver will ensure kwin is started.
# kwrapper5 is used to reduce startup time and memory usage
# kwrapper5 does not return useful error codes such as the exit code of ksmserver.
# We only check for 255 which means that the ksmserver process could not be
# started, any problems thereafter, e.g. ksmserver failing to initialize,
# will remain undetected.
# If the session should be locked from the start (locked autologin),
# lock now and do the rest of the KDE startup underneath the locker.
KSMSERVEROPTIONS=" --no-lockscreen"
kwrapper5 @CMAKE_INSTALL_FULL_BINDIR@/ksmserver $KDEWM $KSMSERVEROPTIONS
if test $? -eq 255; then
  # Startup error
  echo 'startplasma: Could not start ksmserver. Check your installation.'  1>&2
  test -n "$ksplash_pid" && kill "$ksplash_pid" 2>/dev/null
  xmessage -geometry 500x100 "Could not start ksmserver. Check your installation."
fi

#Anything after here is logout
#It is not called after shutdown/restart

wait_drkonqi=`kreadconfig5 --file startkderc --group WaitForDrKonqi --key Enabled --default true`

if test x"$wait_drkonqi"x = x"true"x ; then
    # wait for remaining drkonqi instances with timeout (in seconds)
    wait_drkonqi_timeout=`kreadconfig5 --file startkderc --group WaitForDrKonqi --key Timeout --default 900`
    wait_drkonqi_counter=0
    while qdbus | grep "^[^w]*org.kde.drkonqi" > /dev/null ; do
        sleep 5
        wait_drkonqi_counter=$((wait_drkonqi_counter+5))
        if test "$wait_drkonqi_counter" -ge "$wait_drkonqi_timeout" ; then
            # ask remaining drkonqis to die in a graceful way
            qdbus | grep 'org.kde.drkonqi-' | while read address ; do
                qdbus "$address" "/MainApplication" "quit"
            done
            break
        fi
    done
fi

echo 'startplasma: Shutting down...'  1>&2
# just in case
test -n "$ksplash_pid" && kill "$ksplash_pid" 2>/dev/null

# Clean up
kdeinit5_shutdown

unset KDE_FULL_SESSION
xprop -root -remove KDE_FULL_SESSION
unset KDE_SESSION_VERSION
xprop -root -remove KDE_SESSION_VERSION
unset KDE_SESSION_UID

echo 'startplasma: Done.'  1>&2
