#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSet>
#include <QUrl>

class QWidget;
class QTemporaryDir;

namespace KFI
{
class CInstaller
{
public:
    CInstaller(QWidget *p)
        : m_parent(p)
        , m_tempDir(nullptr)
    {
    }
    ~CInstaller();

    int install(const QSet<QUrl> &urls);

private:
    QWidget *m_parent;
    QTemporaryDir *m_tempDir;
};

}
