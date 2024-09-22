#compdef plasmawindowed

# SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
# SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# keep this function in sync with plasmoidviewer from plasma-sdk
function _plasma_list_packages() {
  local expl global_dir descr="$1" purpose="$2" type="${3:-Plasma/Applet}" kwdarg="$4"
  local -a packages containments
  # Then strip " -- " prefix and " *" suffix (i.e. everything after and including first whitespace).
  # (@) array Expansion Flags are needed because we need to operate on each element separately.
  packages=( ${(f)"$(
    _call_program plasma-list-packages \
      kpackagetool6 --type "$type" --list --global
    )"} )
  # First line is an output header, it contains path where plugins are probed.
  global_dir="${${${packages[1]}##* in }%/}"
  packages[1]=()

  if [[ "$purpose" == "containment" ]]; then
    # No need to get jq involved. We simply look for a magic string, which
    # hopefully won't be named literally in any other irrelevant field.
    containments=( ${${${(f)"$(
      # Glob Qualifiers (.) and (@r) match plain files and readable symbolic links;
      # (N) sets the NULL_GLOB option for the current pattern, so it doesn't print
      # an error for non-existent and empty directories.
      grep --files-with-matches -R "Plasma/Containment" $global_dir/*/metadata.json(N.,@r)
    )"}#$global_dir/}%/metadata.json} )  # strip back prefix and suffix
    packages=( $containments )  # directory name is the package id
  else
    # Don't filter containments out in an 'else' branch: they might be perfectly
    # executable applets on their own, e.g. org.kde.plasma.folder view.
  fi

  if $kwdarg; then
    _wanted "$descr" expl "$purpose" \
      compadd -a packages
  else
    compadd -a packages
  fi
}

_arguments -s \
  '(- *)'{-h,--help}'[Displays help on commandline options]' \
  '(- *)'{-v,--version}'[Displays version information]' \
  '(-d --qmljsdebugger)'{-d,--qmljsdebugger}'[Enable QML Javascript debugger]' \
  '--statusnotifier[Makes the plasmoid stay alive in the Notification Area, even when the window is closed]' \
  '(-p --shell-plugin)'{-p=,--shell-plugin=}'[Force loading the given shell plugin]::: _plasma_list_packages plugins "shell plugin" Plasma/Shell true' \
  \
  '*:: :->applet'

case $state in
  applet)
    _plasma_list_packages applets "applet" Plasma/Applet false
  ;;
esac
