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

#include "msvc_generator.h"

MsvcGenerator::MsvcGenerator(const Process& process)
    : AbstractBTGenerator(process)
{}

bool MsvcGenerator::Init()
{
    return true;
}

void MsvcGenerator::UnInit()
{
}

QString MsvcGenerator::GetFunctionName()
{
    PSYMBOL_INFO symbol =
        (PSYMBOL_INFO) malloc(sizeof(SYMBOL_INFO) + MAX_SYMBOL_NAME);
    ZeroMemory(symbol, sizeof(symbol) + MAX_SYMBOL_NAME);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYMBOL_NAME;

    DWORD64 dwDisplacement = 0;

    if (!SymFromAddr(m_process.GetHandle(), m_currentFrame.AddrPC.Offset, &dwDisplacement, symbol))
    {
        kError() << "SymFromAddr() failed: " << GetLastError();
        return QString::fromLatin1(DEFAULT_FUNC);
    }

    char undecoratedName[MAX_PATH] = {0};
    if (!UnDecorateSymbolName(symbol->Name, undecoratedName, MAX_PATH, UNDNAME_COMPLETE))
    {
        // if this fails, show the decorated name anyway, don't fail
        kError() << "UnDecorateSymbolName() failed: " << GetLastError();
        return QString::fromLatin1(symbol->Name);
    }

    return QString::fromLatin1(undecoratedName);
}

QString MsvcGenerator::GetFile()
{
    IMAGEHLP_LINE64 line;
    ZeroMemory(&line, sizeof(line));
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD dwDisplacement = 0;

    if (!SymGetLineFromAddr64(m_process.GetHandle(), m_currentFrame.AddrPC.Offset, &dwDisplacement, &line))
    {
        kError() << "SymGetLineFromAddr64 failed: " << GetLastError();
        return QString::fromLatin1(DEFAULT_FILE);
    }

    return QString(line.FileName);
}

int MsvcGenerator::GetLine()
{
    IMAGEHLP_LINE64 line;
    ZeroMemory(&line, sizeof(line));
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD dwDisplacement = 0;

    if (!SymGetLineFromAddr64(m_process.GetHandle(), m_currentFrame.AddrPC.Offset, &dwDisplacement, &line))
    {
        //kError() << "SymGetLineFromAddr64 failed: " << GetLastError();
        return DEFAULT_LINE;
    }

    return (int) line.LineNumber;
}

void MsvcGenerator::LoadSymbol(const QString& module, DWORD64 dwBaseAddr)
{
    QString strOutput;

    IMAGEHLP_MODULE64 moduleInfo;
    ZeroMemory(&moduleInfo, sizeof(moduleInfo));
    moduleInfo.SizeOfStruct = sizeof(moduleInfo);
    SymGetModuleInfo64(m_process.GetHandle(), dwBaseAddr, &moduleInfo);

    m_symbolsMap[module] = false; // default
    QString symbolType;
    switch (moduleInfo.SymType)
    {
    case SymNone:
        symbolType = QString::fromLatin1("no symbols loaded");
        break;
    case SymCoff:
    case SymCv:
    case SymPdb:
    case SymSym:
    case SymDia:
        symbolType = QString::fromLatin1("symbols loaded");
        m_symbolsMap[module] = true;
        break;
    case SymExport:
        symbolType = QString::fromLatin1("export table only");
        break;
    case SymDeferred:
        symbolType = QString::fromLatin1("deferred (not loaded currently)");
        break;
    case SymVirtual:
        symbolType = QString::fromLatin1("virtual");
        break;
    }

    strOutput = QString::fromLatin1("Loaded %1 (%2)")
        .arg(module).arg(symbolType);

    emit DebugLine(strOutput);
}
