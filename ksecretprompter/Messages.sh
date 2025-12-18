#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

# Ensure passing shellcheck:
# Our l10n scripting isn't working with spaces anywhere and we actively rely on word splitting in our Messages.sh.
# shellcheck disable=SC2046
podir=${podir:?} # ensure it is defined

$XGETTEXT $(find . -name \*.cpp -o -name \*.h) -o "$podir"/ksecretprompter.pot
