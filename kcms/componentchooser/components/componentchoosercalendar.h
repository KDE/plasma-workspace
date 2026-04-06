/*
    SPDX-FileCopyrightText: 2026 Denys Madureira <denys.madureira@pm.me>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "componentchooser.h"

class ComponentChooserCalendar : public ComponentChooser
{
public:
    ComponentChooserCalendar(QObject *parent);

    QStringList mimeTypes() const override;
};
