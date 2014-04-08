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

#include "abstract_generator.h"

const static DWORD      MAX_SYMBOL_NAME     = 256 * sizeof(TCHAR);

/**
 * \brief Generator for MSVC.
 *
 * This class implements a backtrace generator for executables created with Microsoft's
 * Visual C++. The fundamental difference of executables created with MSVC and MinGW is
 * the debugging format. MSVC uses a proprietary debugging format called PDB (Program
 * Database), which can be manipulated with DbgHelp API.
 *
 */
class MsvcGenerator : public AbstractBTGenerator
{
    Q_OBJECT
public:
    /// Constructor
    MsvcGenerator(const Process& process);

    virtual bool Init();
    virtual void UnInit();

    virtual void FrameChanged() {};

    virtual QString GetFunctionName();
    virtual QString GetFile();
    virtual int     GetLine();

    virtual void LoadSymbol(const QString& module, DWORD64 dwBaseAddr);
};
