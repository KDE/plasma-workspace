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

#include "mingw_generator.h"
#include <bfd.h>
#include <cxxabi.h>

MingwGenerator::MingwGenerator(const Process& process)
    : AbstractBTGenerator(process), file(NULL), func(NULL), line(0) 
{}

struct MyBFD
{
    QString module;
    bfd* abfd;
    asymbol** syms;
    MyBFD() : abfd(0), syms(0)
    {}
    MyBFD(const QString& module, bfd* abfd, asymbol** syms)
    { this->module = module; this->abfd = abfd; this->syms = syms; }
    bool operator==(const MyBFD& other)
    { return module == other.module; }
};

typedef QList<MyBFD> TBFDList;
TBFDList bfds;

asection* text = NULL;

bool MingwGenerator::Init()
{
    bfd_init();
    return true;
}

void MingwGenerator::UnInit()
{
}

void MingwGenerator::FrameChanged()
{
    QString modPath = GetModulePath();
    bool existsSymbol = false;
    TSymbolsMap::const_iterator i = m_symbolsMap.find(modPath);
    if (i == m_symbolsMap.end())
    {
        return;
    }
    MyBFD dummy(modPath, NULL, NULL);
    int pos = bfds.indexOf(dummy);
    if (pos == -1)
    {
        return;
    }
    MyBFD bfd = bfds[pos];
    text = bfd_get_section_by_name(bfd.abfd, ".text");
    long offset = m_currentFrame.AddrPC.Offset - text->vma;
    file = DEFAULT_FILE;
    func = DEFAULT_FUNC;
    line = DEFAULT_LINE;
    if (offset > 0)
    {
        bfd_find_nearest_line(bfd.abfd, text, bfd.syms, offset, &file, &func, (unsigned int*) &line);
    }
}

QString MingwGenerator::GetFunctionName()
{
    if (func != NULL)
    {
        char* realname = abi::__cxa_demangle(func, NULL, NULL, NULL);
        if (realname != NULL)
        {
            QString strReturn = QString::fromLatin1(realname);
            free(realname);
            return strReturn;
        }
        else
        {
            return QString::fromLatin1(func);
        }
    }
    return QString::fromLatin1(DEFAULT_FUNC);
}

QString MingwGenerator::GetFile()
{
    if (file != NULL)
    {
        return QString::fromLatin1(file);
    }
    return QString::fromLatin1(DEFAULT_FILE);
}

int MingwGenerator::GetLine()
{
    if (line > 0)
    {
        return line;
    }
    return -1;
}

void MingwGenerator::LoadSymbol(const QString& module, DWORD64 dwBaseAddr)
{
    QString symbolFile = module;
    symbolFile.truncate(symbolFile.length() - 4);
    symbolFile.append(".sym");

    m_symbolsMap[module] = false; // default
    QString symbolType;
    do
    {
        bfd* abfd = bfd_openr(symbolFile.toLatin1(), NULL);
        if (abfd == NULL)
        {
            symbolType = QString::fromLatin1("no symbols loaded");
            break;
        }
        bfd_check_format(abfd, bfd_object);
        unsigned storage_needed = bfd_get_symtab_upper_bound(abfd);
        assert(storage_needed > 4);
        if (storage_needed <= 4)
        {
            // i don't know why the minimum value for this var is 4...
            symbolType = QString::fromLatin1("no symbols loaded");
            break;
        }
        asymbol** syms = (asymbol **) malloc(storage_needed);
        assert(syms);
        if (syms == NULL)
        {
            symbolType = QString::fromLatin1("no symbols loaded");
            break;
        }
        symbolType = QString::fromLatin1("symbols loaded");
        m_symbolsMap[module] = true;

        bfds.push_back(MyBFD(module, abfd, syms));
    }
    while (0);

    QString strOutput = QString::fromLatin1("Loaded %1 (%2)")
        .arg(module).arg(symbolType);
    emit DebugLine(strOutput);
}
