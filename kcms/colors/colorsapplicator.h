/*
   Copyright (c) 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>
   Copyright (c) 2021 Benjamin Port <benjamin.port@enioka.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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
