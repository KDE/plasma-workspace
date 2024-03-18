#! /usr/bin/env bash
$XGETTEXT "$(find . -name \*.cpp -o -name \*.h)" -o $podir/plasma_interactiveconsole.pot
