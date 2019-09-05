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

#include <QCoreApplication>
#include <QDebug>
#include "../autostart.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    AutoStart as;

    QTextStream cout(stdout);
    auto printPhase = [&cout, &as] (int phase) -> bool {
        AutoStart asN(as);
        asN.setPhase(phase);
        cout << "phase: " << phase << '\n';
        bool foundThings = true;
        for (auto asi : asN.startList()) {
            foundThings = false;
            cout << "- " << asi.name << ' ' << asi.service;
            if (!asi.startAfter.isEmpty())
                cout << ", startAfter:" << asi.startAfter;
            cout << '\n';
        }
        cout << '\n';
        return !foundThings;
    };

    printPhase(0);
    printPhase(1);
    printPhase(2);
    return 0;
}
