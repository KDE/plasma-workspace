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

#include "abstract_generator.h"
#include "callbacks.h"

#include <QStringList>

AbstractBTGenerator::AbstractBTGenerator(const Process& process)
    : m_process(process)
{
    assert(process.IsValid());
}

AbstractBTGenerator::~AbstractBTGenerator()
{
}

QString AbstractBTGenerator::GetModuleName()
{
    IMAGEHLP_MODULE64 module;
    ZeroMemory(&module, sizeof(module));
    module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

    if (!SymGetModuleInfo64(m_process.GetHandle(), m_currentFrame.AddrPC.Offset, &module))
    {
        kError() << "SymGetModuleInfo64 failed: " << GetLastError();
        return QLatin1String(DEFAULT_MODULE);
    }

    QStringList list = QString(module.ImageName).split("\\");
    return list[list.size() - 1];
}

QString AbstractBTGenerator::GetModulePath()
{
    IMAGEHLP_MODULE64 module;
    ZeroMemory(&module, sizeof(module));
    module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

    if (!SymGetModuleInfo64(m_process.GetHandle(), m_currentFrame.AddrPC.Offset, &module))
    {
        kError() << "SymGetModuleInfo64 failed: " << GetLastError();
        return QLatin1String(DEFAULT_MODULE);
    }

    return QString(module.ImageName);
}

void AbstractBTGenerator::Run(HANDLE hThread, bool bFaultingThread)
{
    assert(m_process.IsValid());
    assert(hThread);

    if (!Init())
    {
        assert(false);
        return;
    }

    //HANDLE hFile = CreateFile(L"C:\\test\\test.dmp", FILE_ALL_ACCESS, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    //if (!MiniDumpWriteDump(m_process.GetHandle(), m_process.GetId(), hFile,
    //    MiniDumpNormal, NULL, NULL, NULL))
    //{
    //    HRESULT hres = (HRESULT) GetLastError();
    //    printf("%08X\n\n", hres);
    //}
    //SafeCloseHandle(hFile);

    DWORD dw = SuspendThread(hThread);
    assert(dw != DWORD(-1));
    if (dw == DWORD(-1))
    {
        kError() << "SuspendThread() failed: " << GetLastError();
        return;
    }

    CONTEXT context;
    ZeroMemory(&context, sizeof(context));
    if (!bFaultingThread)
    {
        // if it's not the faulting thread, get its context
        context.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(hThread, &context))
        {
            ResumeThread(hThread);
            assert(false);
            kError() << "GetThreadContext() failed: " << GetLastError();
            return;
        }
    }
    else
    {
        // if it is, get it from KCrash
        HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, L"Local\\KCrashShared");
        if (hMapFile == NULL)
        {
            kError() << "OpenFileMapping() failed: " << GetLastError();
            return;
        }
        CONTEXT *othercontext = (CONTEXT*) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CONTEXT));
        if (othercontext == NULL)
        {
            kError() << "MapViewOfFile() failed: " << GetLastError();
            SafeCloseHandle(hMapFile);
            return;
        }
        CopyMemory(&context, othercontext, sizeof(CONTEXT));
        UnmapViewOfFile(othercontext); // continue even if it fails
        SafeCloseHandle(hMapFile);
    }

    // some of this stuff is taken from StackWalker
    ZeroMemory(&m_currentFrame, sizeof(m_currentFrame));
    DWORD machineType = IMAGE_FILE_MACHINE_UNKNOWN;
#if defined(_M_IX86)
    machineType = IMAGE_FILE_MACHINE_I386;
    m_currentFrame.AddrPC.Offset = context.Eip;
    m_currentFrame.AddrFrame.Offset = context.Ebp;
    m_currentFrame.AddrStack.Offset = context.Esp;
#elif defined(_M_X64)
    machineType = IMAGE_FILE_MACHINE_AMD64;
    m_currentFrame.AddrPC.Offset = context.Rip;
    m_currentFrame.AddrFrame.Offset = context.Rbp;
    m_currentFrame.AddrStack.Offset = context.Rsp;
#else
# error This architecture is not supported.
#endif
    m_currentFrame.AddrPC.Mode = AddrModeFlat;
    m_currentFrame.AddrFrame.Mode = AddrModeFlat;
    m_currentFrame.AddrStack.Mode = AddrModeFlat;

    SymSetOptions(SymGetOptions() | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
    SymInitialize(m_process.GetHandle(), NULL, FALSE);

    LoadSymbols();

    for (int i = 0; /*nothing*/; i++)
    {
        SetLastError(0);

        if (!StackWalk64(
            machineType,
            m_process.GetHandle(),
            hThread,
            &m_currentFrame,
            &context,
            &Callbacks::ReadProcessMemory,
            &Callbacks::SymFunctionTableAccess64,
            &Callbacks::SymGetModuleBase64,
            NULL))
        {
            emit Finished();
            kDebug() << "Stackwalk finished; GetLastError=" << GetLastError();
            break;
        }
        
        FrameChanged();

        QString modulename = GetModuleName();
        QString functionname = GetFunctionName();
        QString file = GetFile();
        int line = GetLine();
        QString address = QString::number(m_currentFrame.AddrPC.Offset, 16);

        QString debugLine = QString::fromLatin1(BACKTRACE_FORMAT).
            arg(modulename).arg(functionname).arg(file).arg(line).arg(address);
        
        emit DebugLine(debugLine);
    }

    // Resume the target thread now, or else the crashing process will not
    // be terminated
    ResumeThread(hThread);

    SymCleanup(m_process.GetHandle());
}

bool AbstractBTGenerator::IsSymbolLoaded(const QString& module)
{
    if (m_symbolsMap.contains(module))
    {
        return m_symbolsMap[module];
    }
    return false;
}

void AbstractBTGenerator::LoadSymbols()
{
    TModulesMap modules = m_process.GetModules();
    for (TModulesMap::iterator i = modules.begin(); i != modules.end(); i++)
    {
        MODULEINFO modInfo;
        ZeroMemory(&modInfo, sizeof(modInfo));

        QString strModule = i.key();

        GetModuleInformation(m_process.GetHandle(), i.value(), &modInfo, sizeof(modInfo));
        SymLoadModuleEx(
            m_process.GetHandle(),
            NULL,
            (CHAR*) i.key().toLatin1().constData(),
            (CHAR*) i.key().toLatin1().constData(),
            (DWORD64) modInfo.lpBaseOfDll,
            modInfo.SizeOfImage,
            NULL,
            0);

        LoadSymbol(strModule, (DWORD64) modInfo.lpBaseOfDll);

        if (!IsSymbolLoaded(strModule))
        {
            emit MissingSymbol(strModule);
        }
    }
    emit DebugLine(QString());
    emit DebugLine(QString());
}
