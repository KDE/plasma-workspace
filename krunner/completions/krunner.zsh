#compdef krunner

# SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
#
# SPDX-License-Identifier: GPL-2.0-or-later

function _krunner_runners() {
  local -a runners lines split
  local line

  # Fetch list, strip header, split by lines
  lines=( ${(f)"$(_call_program krunner-runners krunner --list | tail -n +3)"} )

  for line in ${lines[@]}; do
    # Split by ": " (including whitespace) and swap the order to the format expected by _describe
    split=( ${(s/: /)line} )
    runners+=( "${split[2]}:${split[1]}" )
  done

  _describe -t runner 'runner' runners
}

local ret=1

_arguments -C \
  '(* -)'{-h,--help}'[Displays help on commandline options]' \
  '(-c --clipboard)'{-c,--clipboard}'[Use the clipboard contents as query for KRunner]' \
  '(-d --daemon)'{-d,--daemon}"[Start KRunner in the background, don't show it]" \
  '--replace[Replace an existing instance]' \
  '--runner=[Show only results from the given plugin]:runner:_krunner_runners' \
  '--list[List available plugins]' \
  '*:query:' \
  && ret=0

return $ret
