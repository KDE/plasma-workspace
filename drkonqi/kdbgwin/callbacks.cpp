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

#include "callbacks.h"

BOOL Callbacks::ReadProcessMemory(HANDLE hProcess, DWORD64 qwBaseAddr, PVOID lpBuf, DWORD nSize, LPDWORD lpBytesRead)
{
    SIZE_T st;
    BOOL bRet = ::ReadProcessMemory(hProcess, (LPVOID) qwBaseAddr, (LPVOID) lpBuf, nSize, &st);
    *lpBytesRead = (DWORD) st;
    //kDebug() << "bytes read=" << st << "; bRet=" << bRet << "; LastError: " << GetLastError();
    return bRet;
}

PVOID Callbacks::SymFunctionTableAccess64(HANDLE hProcess, DWORD64 qwAddr)
{
    PVOID ret = ::SymFunctionTableAccess64(hProcess, qwAddr);
    //kDebug() << "ret=" << ret << "; LastError: " << GetLastError();
    return ret;
}

DWORD64 Callbacks::SymGetModuleBase64(HANDLE hProcess, DWORD64 qwAddr)
{
    DWORD64 ret = ::SymGetModuleBase64(hProcess, qwAddr);
    //kDebug() << "ret=" << ret << "; LastError: " << GetLastError();
    return ret;
}
