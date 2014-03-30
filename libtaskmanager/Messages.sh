#! /usr/bin/env bash
$EXTRACTRC *.ui  >> rc.cpp || exit 11
$XGETTEXT *.cpp */*.cpp *.h -o $podir/libtaskmanager.pot
rm -f rc.cpp
