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

typedef QMap<DWORD, HANDLE>     TThreadsMap;
typedef QMap<QString, HMODULE>   TModulesMap;

/**
 * \brief Describes a process.
 *
 * This is a helper class when dealing with another process. When kdbgwin starts,
 * it attaches to the crashing process and tries to retrieve useful information:
 * pid, threads, modules, image path. These will be used later.
 */
class Process
{
private:
    /// Flag to check if the information about this process is valid and can be used
    BOOL        m_bValid;

    /// Process ID
    DWORD       m_dwPid;

    /// Failing thread ID - I need this because for the crashing thread, I need to get
    /// the CONTEXT from a piece of shared memory in KCrash
    DWORD       m_dwThread;

    /// A handle to the process
    HANDLE      m_hProcess;

    /// A QMap<DWORD, HANDLE> which associates thread IDs with opened handles for each
    /// of them
    TThreadsMap m_threads;

    /// The full path to the executable file which started this process
    QString     m_path;

    /// A QMap<QString, HMODULE> which contains the paths to the loaded modules and
    /// handles to each of them
    TModulesMap m_modules;

public:
    Process();

public:
    /// kdbgwin needs to enable the debug privilege in order to read from
    /// another process's memory.
    static BOOL EnableDebugPrivilege();

public:
    /// Attaches to the process and gets all required information
    /// @return TRUE if operation succeeds
    BOOL GetInfo(const char* pid, const char* threadId);

    /// Checks if the information is valid
    BOOL IsValid() const
    { assert(m_bValid); return m_bValid; }

    /// Get the process ID
    DWORD GetId() const
    { assert(m_dwPid); return m_dwPid; }

    /// Returns an open handle to the process (opened with PROCESS_ALL_ACCESS)
    HANDLE GetHandle() const
    { assert(m_hProcess); return m_hProcess; }

    /// Returns the thread ID of the thread that caused the exception
    DWORD GetThreadId() const
    { assert(m_dwThread); return m_dwThread; }

    /// Returns the threads map
    const TThreadsMap& GetThreads() const
    { return m_threads; }

    /// Returns the full path to the executable on the disk
    const QString& GetPath() const
    { return m_path; }

    /// Returns a map of all the loaded modules
    const TModulesMap& GetModules() const
    { return m_modules; }
};
