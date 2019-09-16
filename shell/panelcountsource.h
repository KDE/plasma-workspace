/*
 *   Copyright 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PANELCOUNTSOURCE_H
#define PANELCOUNTSOURCE_H

#include "shellcorona.h"
#include <KLocalizedString>
#include <KUserFeedback/AbstractDataSource>

class PanelCountSource : public KUserFeedback::AbstractDataSource
{
public:
    /*! Create a new start count data source. */
    PanelCountSource(ShellCorona* corona)
        : AbstractDataSource(QStringLiteral("panelCount"), KUserFeedback::Provider::DetailedSystemInformation)
        , corona(corona)
    {}

    QString name() const override { return i18n("Panel Count"); }
    QString description() const override { return i18n("Counts the panels"); }

    QVariant data() override { return corona->panelCount(); }

private:
    ShellCorona* const corona;
};

#endif
