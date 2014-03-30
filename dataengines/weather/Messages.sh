#! /usr/bin/env bash
for file in ions/data/*.dat
do
  awk -F'|' '$0 ~ /\|/ {
                         print "// i18n: file: '`basename $file`':"NR;
                         printf("i18nc(\"%s\", \"%s\");\n", $1, $2)
                       }' $file >> rc.cpp
done

$XGETTEXT `find . -name \*.cpp` -o $podir/plasma_engine_weather.pot
