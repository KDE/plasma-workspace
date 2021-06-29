/*
    SPDX-FileCopyrightText: 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef COLORSAPPLICATOR_H
#define COLORSAPPLICATOR_H

#include <QString>

#include <KConfig>

/**
 * Performs the task of actually applying a color scheme to the current session, based on
 * color scheme file path and configuration file.
 * When using this function, you select the scheme to use by setting the model's selected scheme
 * @param colorFilePath The scheme color file path
 * @param configOut The config which holds the information on which scheme is currently selected, and what colors it contains
 */
void applyScheme(const QString &colorSchemePath, KConfig *configOut, KConfig::WriteConfigFlags writeFlags = KConfig::Normal);

#endif // COLORSAPPLICATOR_H
