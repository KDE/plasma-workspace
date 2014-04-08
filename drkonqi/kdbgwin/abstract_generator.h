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

#include "common.h"
#include "process.h"

const static char*      BACKTRACE_FORMAT    = "%1!%2() [%3 @ %4] at 0x%5";
// module.dll!KClass::function() [c:\file.cpp @ 10] at 0x0001000

const static char*      DEFAULT_MODULE  = "[unknown]";
const static char*      DEFAULT_FUNC    = "[unknown]";
const static char*      DEFAULT_FILE    = "[unknown]";
const static int        DEFAULT_LINE    = -1;

/**
 * \brief Base generator class
 *
 * This class gives the definition of a backtrace generator. There are 2 subclasses: one for
 * MSVC and one for MinGW. The reason why implementation differs is the fact that executables
 * use different debugging formats: PDBs (for MSVC) and Dwarf-2/Stabs and possibly more I don't
 * know too much about for MinGW, which are embedded in the executable itself.
 */
class AbstractBTGenerator : public QObject
{
    Q_OBJECT
protected:
    /// A Process instance, corresponding to the process for which we generate the backtrace
    Process m_process;

    /// The current stack frame. It is kept as a member 
    STACKFRAME64 m_currentFrame;

    /// The definition of a map of symbols
    typedef QMap<QString, bool> TSymbolsMap;
    /// A map of symbols (the full path to the module, and a bool which specifies if
    /// symbols were loaded for it)
    TSymbolsMap m_symbolsMap;

public:
    /// Constructor
    AbstractBTGenerator(const Process& process);
    virtual ~AbstractBTGenerator();

    /// Abstract virtual: Initialize this generator
    virtual bool Init() = 0;

    /// Abstract virtual: Uninitialize this generator
    virtual void UnInit() = 0;

    /// Start generating the backtrace
    virtual void Run(HANDLE hTread, bool bFaultingThread);

    /// This method acts like a callback and will be called when the stack frame changed
    virtual void FrameChanged() = 0;

    /// Abstract virtual: get current module name
    /// @return the name of the current module (eg: kdecore.dll)
    virtual QString GetModuleName();

    /// Abstract virtual: get current module path
    /// @return the full path to the current module (eg: N:\\kde\\bin\\kdecore.dll)
    virtual QString GetModulePath();

    /// Abstract virtual: get current function/method name. The name is undecorated internally by this method.
    /// @return the current function or method (eg: KCmdLineArgs::args)
    virtual QString GetFunctionName() = 0;

    /// Abstract virtual: get the full path to the current file
    /// @return the path to the file (eg: N:\\kde\\svn\\KDE\\trunk\\kdelibs\\kcmdlineargs\\kcmdlineargs.cpp)
    virtual QString GetFile() = 0;

    /// Abstract virtual: get current line
    /// @return the current line in the file
    virtual int     GetLine() = 0;

    /// Checks if symbols are loaded for the specified module
    /// @return true if symbols are loaded
    virtual bool IsSymbolLoaded(const QString& module);

    /// Tries to load symbols for all loaded modules
    virtual void LoadSymbols();

    /// Tries to load a symbol file for a module loaded at dwBaseAddr
    virtual void LoadSymbol(const QString& module, DWORD64 dwBaseAddr) = 0;
signals:
    /// This will be emitted whenever the generator wishes to output information. It can either be
    /// module information (in the form: "Loaded C:\path\to.dll (symbols loaded)", a stack frame line,
    /// or newlines
    void DebugLine(const QString&);

    /// This signal is emitted when a module is loaded, and its symbols are missing. This is
    /// caught by the PackageSuggester
    void MissingSymbol(const QString&);

    /// Will be emitted when the generation finishes
    void Finished();
};
