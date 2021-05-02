/*
   Copyright (c) 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>

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

class ColorsSettings;
class ColorsModel;
/**
 * Performs the task of actually applying a color scheme to the current session, based on
 * what is currently set in the settings and model instances passed into the function.
 * When using this function, you select the scheme to use by setting the model's selected scheme
 * @param settings The settings instance which lets us update the system with the new colors
 * @param model The model which holds the information on which scheme is currently selected, and what colors it contains
 * @see ColorsModel::setSelectedScheme(QString)
 */
void applyScheme(ColorsSettings *settings, ColorsModel *model);

#endif // COLORSAPPLICATOR_H
