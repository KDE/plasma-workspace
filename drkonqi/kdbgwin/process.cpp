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

#include "common.h"
#include "process.h"

Process::Process()
    : m_bValid(FALSE)
{
}

// we need debug privileges to open the proces with PROCESS_ALL_ACCESS, and
// to successfully use ReadProcessMemory()
BOOL Process::EnableDebugPrivilege()
{
    kDebug() << "Enabling debug privilege";
    HANDLE hToken = NULL;

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
    {
        if (GetLastError() == ERROR_NO_TOKEN)
        {
            if (!ImpersonateSelf(SecurityImpersonation))
            {
                kError() << "ImpersonateSelf() failed: " << GetLastError();
                return FALSE;
            }
            if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
            {
                kError() << "OpenThreadToken() #2 failed: " << GetLastError();
                return FALSE;
            }
        }
        else
        {
            kError() << "OpenThreadToken() #1 failed: " << GetLastError();
            return FALSE;
        }
    }

    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
    {
        assert(false);
        kError() << "Cannot lookup privilege: " << GetLastError();
        SafeCloseHandle(hToken);
        return FALSE;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    tp.Privileges[0].Luid = luid;
    
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL))
    {
        assert(false);
        kError() << "Cannot adjust privilege: " << GetLastError();
        SafeCloseHandle(hToken);
        return FALSE;
    }

    SafeCloseHandle(hToken);
    return TRUE;
}

BOOL Process::GetInfo(const char* pid, const char* threadId)
{
    kDebug() << "Trying to get info about pid=" << pid;

    DWORD dwPid = DWORD(atoi(pid));
    DWORD dwThread = DWORD(atoi(threadId));

    // get handle to the process
    HANDLE hProcess = NULL;
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
    assert(hProcess);
    if (hProcess == NULL)
    {
        kError() << "Cannot open process " << dwPid << ": " << GetLastError();
        return m_bValid;
    }
    m_dwPid = dwPid;
    m_hProcess = hProcess;
    m_dwThread = dwThread;

    TCHAR procPath[MAX_PATH * 2 + 1] = {0};
    GetModuleFileNameEx(hProcess, NULL, procPath, MAX_PATH*2 + 1);
    m_path = QString::fromWCharArray(procPath);

    // we can't get the threads for a single process, so get all system's
    // threads, and enumerate through them
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, NULL);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        kError() << "CreateToolhelp32Snapshot() failed: " << GetLastError();
        assert(false);
        return m_bValid;
    }

    // get process threads
    THREADENTRY32 te;
    ZeroMemory(&te, sizeof(te));
    te.dwSize = sizeof(te);
    if (Thread32First(hSnapshot, &te))
    {
        do 
        {
            if (te.th32OwnerProcessID == dwPid)
            {
                kDebug() << "Found thread " << te.th32ThreadID << ", adding to list";
                
                HANDLE hThread = NULL;
                hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                assert(hThread);
                if (hThread == NULL)
                {
                    kError() << "Cannot open thread " << te.th32ThreadID << ": " << GetLastError();
                    continue;
                }
                
                m_threads[te.th32ThreadID] = hThread;
                // we have at least 1 thread, make this valid
                m_bValid = TRUE;
            }
        } 
        while (Thread32Next(hSnapshot, &te));
    }
    SafeCloseHandle(hSnapshot);

    assert(m_threads.size() > 0);

    // get process modules
    HMODULE hMods[1024];
    DWORD cbNeeded = 0;
    if (!EnumProcessModules(hProcess, hMods, ArrayCount(hMods), &cbNeeded))
    {
        kError() << "Cannot enumerate modules: " << GetLastError();
        return m_bValid;
    }
    for (size_t i = 0; i < (cbNeeded / sizeof(hMods[0])); i++)
    {
        /*
         * In Windows, a wchar_t has 2 bytes; GCC defines wchar_t as int,
         * which is 4 bytes; so i can't use TCHAR here; better off using ushort
         * and casting when necessary
         */
        ushort szModName[MAX_PATH];
        if (GetModuleFileNameEx(hProcess, hMods[i], (LPTSTR) szModName, MAX_PATH))
        {
            //QString str = QString::fromUtf16(szModName);
            //kDebug() << "Got module: " << str;
            //m_modules.push_back(QString::fromUtf16(szModName));
            m_modules[QString::fromUtf16(szModName)] = hMods[i];
        }
    }

    return m_bValid;
}
