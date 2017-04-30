#! /usr/bin/env bash
$XGETTEXT `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $podir/kio5_applications.pot
