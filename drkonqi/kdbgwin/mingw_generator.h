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

/**
 * \brief Generator for MinGW.
 *
 * This class allows generating backtraces for executables created with the MinGW compiler.
 * It assumes that symbols are stored in .sym files in the same directory as the corresponding
 * binary (eg. N:\\kde\\bin\\libkdecore.dll and N:\\kde\\bin\\libkdecore.sym). It uses libbfd to
 * find and extract the information it needs from the symbols it loads.
 */
class MingwGenerator : public AbstractBTGenerator
{
    Q_OBJECT
protected:
    /// The current file
    const char* file;
    /// The current function
    const char* func;
    /// The current line
    int line;
public:
    /// Constructor
    MingwGenerator(const Process& process);

    virtual bool Init();
    virtual void UnInit();

    virtual void FrameChanged();

    virtual QString GetFunctionName();
    virtual QString GetFile();
    virtual int     GetLine();

    virtual void LoadSymbol(const QString& module, DWORD64 dwBaseAddr);
};
