/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KCHECKRUNNING_H
#define KCHECKRUNNING_H

enum CheckRunningState {
    PlasmaRunning,
    NoPlasmaRunning,
    NoX11,
};

CheckRunningState kCheckRunning();

#endif
