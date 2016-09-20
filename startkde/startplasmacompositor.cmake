#!/bin/sh
#
#  DEFAULT KDE STARTUP SCRIPT ( @PROJECT_VERSION@ )
#

# in case we have been started with full pathname spec without being in PATH
bindir=`echo "$0" | sed -n 's,^\(/.*\)/[^/][^/]*$,\1,p'`
if [ -n "$bindir" ]; then
  qbindir=`qtpaths --binaries-dir`
  qdbus=$qbindir/qdbus
  case $PATH in
    $bindir|$bindir:*|*:$bindir|*:$bindir:*) ;;
    *) PATH=$bindir:$PATH; export PATH;;
  esac
else
  qdbus=qdbus
fi

# We need to create config folder so we can write startupconfigkeys
if [  ${XDG_CONFIG_HOME} ]; then
  configDir=$XDG_CONFIG_HOME;
else
  configDir=${HOME}/.config; #this is the default, http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
fi

mkdir -p $configDir

#This is basically setting defaults so we can use them with kstartupconfig5
cat >$configDir/startupconfigkeys <<EOF
kcminputrc Mouse cursorTheme 'breeze_cursors'
kcminputrc Mouse cursorSize ''
ksplashrc KSplash Theme Breeze
ksplashrc KSplash Engine KSplashQML
kdeglobals KScreen ScaleFactor 1
kcmfonts General forceFontDPI 0
EOF

# preload the user's locale on first start
plasmalocalerc=$configDir/plasma-localerc
test -f $plasmalocalerc || {
cat >$plasmalocalerc <<EOF
[Formats]
LANG=$LANG
EOF
}

# export LC_* variables set by kcmshell5 formats into environment
# so it can be picked up by QLocale and friends.
exportformatssettings=$configDir/plasma-locale-settings.sh
test -f $exportformatssettings && {
    . $exportformatssettings
}

# Write a default kdeglobals file to set up the font
kdeglobalsfile=$configDir/kdeglobals
test -f $kdeglobalsfile || {
cat >$kdeglobalsfile <<EOF
[General]
XftAntialias=true
XftHintStyle=hintmedium
XftSubPixel=none
EOF
}

# Make sure the Oxygen font is installed
# This is necessary for setups where CMAKE_INSTALL_PREFIX
# is not in /usr. fontconfig looks in /usr, ~/.fonts and
# $XDG_DATA_HOME for fonts. In this case, we symlink the
# Oxygen font under ${XDG_DATA_HOME} and make it known to
# fontconfig

usr_share="/usr/share"
install_share="@KDE_INSTALL_FULL_DATADIR@"

if [ ! $install_share = $usr_share ]; then

    if [ ${XDG_DATA_HOME} ]; then
        fontsDir="${XDG_DATA_HOME}/fonts"
    else
        fontsDir="${HOME}/.fonts"
    fi

    test -d $fontsDir || {
        mkdir -p $fontsDir
    }

    oxygenDir=$fontsDir/truetype/oxygen
    prefixDir="@KDE_INSTALL_FULL_DATADIR@/fonts/truetype/oxygen"

    # if the oxygen dir doesn't exist, create a symlink to be sure that the
    # Oxygen font is available to the user
    test -d $oxygenDir || test -d $prefixDir && {
        test -h $oxygenDir || ln -s $prefixDir $oxygenDir && fc-cache $oxygenDir
    }
fi

kstartupconfig5
returncode=$?
if test $returncode -ne 0; then
    exit 1
fi
[ -r $configDir/startupconfig ] && . $configDir/startupconfig

if [ $kdeglobals_kscreen_scalefactor -gt 1 ]; then
    export QT_SCREEN_SCALE_FACTORS="$kdeglobals_kscreen_scalefactor"
fi
#Manually disable auto scaling because we are scaling above
#otherwise apps that manually opt in for high DPI get auto scaled by the developer AND manually scaled by us
export QT_AUTO_SCREEN_SCALE_FACTOR=0



# XCursor mouse theme needs to be applied here to work even for kded or ksmserver
if test -n "$kcminputrc_mouse_cursortheme" -o -n "$kcminputrc_mouse_cursorsize" ; then
    @EXPORT_XCURSOR_PATH@

    # TODO: is kapplymousetheme a core app?
    #kapplymousetheme "$kcminputrc_mouse_cursortheme" "$kcminputrc_mouse_cursorsize"
    if test $? -eq 10; then
        XCURSOR_THEME=breeze_cursors
        export XCURSOR_THEME
    elif test -n "$kcminputrc_mouse_cursortheme"; then
        XCURSOR_THEME="$kcminputrc_mouse_cursortheme"
        export XCURSOR_THEME
    fi
    if test -n "$kcminputrc_mouse_cursorsize"; then
        XCURSOR_SIZE="$kcminputrc_mouse_cursorsize"
        export XCURSOR_SIZE
    fi
fi

if test "$kcmfonts_general_forcefontdpi" -ne 0; then
    export QT_WAYLAND_FORCE_DPI=$kcmfonts_general_forcefontdpi
fi

# Source scripts found in <config locations>/plasma-workspace/env/*.sh
# (where <config locations> correspond to the system and user's configuration
# directories, as identified by Qt's qtpaths,  e.g.  $HOME/.config
# and /etc/xdg/ on Linux)
#
# This is where you can define environment variables that will be available to
# all KDE programs, so this is where you can run agents using e.g. eval `ssh-agent`
# or eval `gpg-agent --daemon`.
# Note: if you do that, you should also put "ssh-agent -k" as a shutdown script
#
# (see end of this file).
# For anything else (that doesn't set env vars, or that needs a window manager),
# better use the Autostart folder.

# TODO: Use GenericConfigLocation once we depend on Qt 5.4
scriptpath=`qtpaths --paths ConfigLocation | tr ':' '\n' | sed 's,$,/plasma-workspace,g'`

# Add /env/ to the directory to locate the scripts to be sourced
for prefix in `echo $scriptpath`; do
  for file in "$prefix"/env/*.sh; do
    test -r "$file" && . "$file"
  done
done

echo 'startplasmacompositor: Starting up...'  1>&2

# Make sure that the KDE prefix is first in XDG_DATA_DIRS and that it's set at all.
# The spec allows XDG_DATA_DIRS to be not set, but X session startup scripts tend
# to set it to a list of paths *not* including the KDE prefix if it's not /usr or
# /usr/local.
if test -z "$XDG_DATA_DIRS"; then
XDG_DATA_DIRS="@KDE_INSTALL_FULL_DATADIR@:/usr/share:/usr/local/share"
fi
export XDG_DATA_DIRS

# Make sure that D-Bus is running
if $qdbus >/dev/null 2>/dev/null; then
    : # ok
else
    echo 'startplasmacompositor: Could not start D-Bus. Can you call qdbus?'  1>&2
    test -n "$ksplash_pid" && kill "$ksplash_pid" 2>/dev/null
    exit 1
fi


# Mark that full KDE session is running (e.g. Konqueror preloading works only
# with full KDE running). The KDE_FULL_SESSION property can be detected by
# any X client connected to the same X session, even if not launched
# directly from the KDE session but e.g. using "ssh -X", kdesu. $KDE_FULL_SESSION
# however guarantees that the application is launched in the same environment
# like the KDE session and that e.g. KDE utilities/libraries are available.
# KDE_FULL_SESSION property is also only available since KDE 3.5.5.
# The matching tests are:
#   For $KDE_FULL_SESSION:
#     if test -n "$KDE_FULL_SESSION"; then ... whatever
#   For KDE_FULL_SESSION property:
#     xprop -root | grep "^KDE_FULL_SESSION" >/dev/null 2>/dev/null
#     if test $? -eq 0; then ... whatever
#
# Additionally there is (since KDE 3.5.7) $KDE_SESSION_UID with the uid
# of the user running the KDE session. It should be rarely needed (e.g.
# after sudo to prevent desktop-wide functionality in the new user's kded).
#
# Since KDE4 there is also KDE_SESSION_VERSION, containing the major version number.
# Note that this didn't exist in KDE3, which can be detected by its absense and
# the presence of KDE_FULL_SESSION.
#
KDE_FULL_SESSION=true
export KDE_FULL_SESSION

KDE_SESSION_VERSION=5
export KDE_SESSION_VERSION

KDE_SESSION_UID=`id -ru`
export KDE_SESSION_UID

XDG_CURRENT_DESKTOP=KDE
export XDG_CURRENT_DESKTOP

#enforce wayland QPA
QT_QPA_PLATFORM=wayland
export QT_QPA_PLATFORM

# At this point all environment variables are set, let's send it to the DBus session server to update the activation environment
if which dbus-update-activation-environment >/dev/null 2>/dev/null ; then
    dbus-update-activation-environment --systemd --all
else
    @CMAKE_INSTALL_FULL_LIBEXECDIR@/ksyncdbusenv
fi
if test $? -ne 0; then
  # Startup error
  echo 'startplasmacompositor: Could not sync environment to dbus.'  1>&2
  exit 1
fi

@KWIN_WAYLAND_BIN_PATH@ --xwayland --libinput --exit-with-session=@CMAKE_INSTALL_FULL_LIBEXECDIR@/startplasma

echo 'startplasmacompositor: Shutting down...'  1>&2

unset KDE_FULL_SESSION
xprop -root -remove KDE_FULL_SESSION
unset KDE_SESSION_VERSION
xprop -root -remove KDE_SESSION_VERSION
unset KDE_SESSION_UID

echo 'startplasmacompositor: Done.'  1>&2
