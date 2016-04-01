#! /usr/bin/env bash
$XGETTEXT `find . -name \*.cpp` -o $podir/plasma_engine_ion_%{APPNAMELC}.pot
