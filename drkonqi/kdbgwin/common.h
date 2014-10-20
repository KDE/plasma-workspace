/******************************************************************
 *
 * kdbgwin - Helper application for DrKonqi
 *
 * This file is part of the KDE project
 *
 * Copyright (C) 2010 Ilie Halip <lupuroshu@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>
 *****************************************************************/

#pragma once

// the compiler only provides UNICODE. tchar.h checks for the _UNICODE macro
#if defined(UNICODE)
#define _UNICODE
#endif

// first: windows & compiler includes
#include <Tchar.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <Assert.h>
#include <Shlwapi.h>
#include <Psapi.h>
#include <TlHelp32.h>


// second: Qt includes
#include <QString>
#include <QString>


// third: KDE includes


// common defines
#define SafeCloseHandle(h) \
    CloseHandle(h); \
    h = NULL;

#define ArrayCount(x) (sizeof(x) / sizeof(x[0]))




// Documentation
/**
\mainpage KDbgWin

KDbgWin (KDE Debugger for Windows) is a helper application for DrKonqi. Because KDE-Windows supports
2 compilers (MSVC and MinGW), and there is no debugger that supports them both, a simple debugger was needed
to make DrKonqi able to generate backtraces - Windows only.

MSVC generates .pdb files for its binaries, and GNU GCC embeds debugging information in executables. However,
with MinGW, debugging information can be stripped into external files and then loaded on demand. So the only
difference between the two is how symbols are handled. DbgHelp and LibBfd were used for manipulating and getting
the required information from each debugging format.
*/
