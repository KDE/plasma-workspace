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
    CInstaller(const QString &parentWindow)
        : m_parentWindow(parentWindow)
        , m_tempDir(nullptr)
    {
    }
    ~CInstaller();

    int install(const QSet<QUrl> &urls);

private:
    QString m_parentWindow;
    QTemporaryDir *m_tempDir;
};

}
