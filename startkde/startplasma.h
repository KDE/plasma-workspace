/* This file is part of the KDE project
   Copyright (C) 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef STARTPLASMA_H
#define STARTPLASMA_H

#include "kcheckrunning/kcheckrunning.h"
#include <ksplashinterface.h>
#include "config-startplasma.h"

extern QTextStream out;

QStringList allServices(const QLatin1String& prefix);
int runSync(const QString& program, const QStringList &args, const QStringList &env = {});
void sourceFiles(const QStringList &files);
void messageBox(const QString &text);

void createConfigDirectory();
void runStartupConfig();
void setupCursor(bool wayland);
void runEnvironmentScripts();
void setupPlasmaEnvironment();
void cleanupPlasmaEnvironment();
void cleanupX11();
bool syncDBusEnvironment();
void setupFontDpi();
QProcess* setupKSplash();
void setupX11();

bool startKDEInit();
bool startPlasmaSession(bool wayland);

void waitForKonqi();

struct KillBeforeDeleter
{
    static inline void cleanup(QProcess *pointer)
    {
        if (pointer)
            pointer->kill();
        delete pointer;
    }
};

#endif
