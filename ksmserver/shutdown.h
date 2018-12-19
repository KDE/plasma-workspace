/*****************************************************************
ksmserver - the KDE session management server

Copyright 2018 David Edmundson <davidedmundson@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#pragma once

#include <QObject>
#include <kworkspace.h>

class Shutdown: public QObject
{
    Q_OBJECT
public:
    Shutdown(QObject *parent = nullptr);
    void logout();
    void logoutAndShutdown();
    void logoutAndReboot();
private Q_SLOTS:
    void logoutCancelled();
    void logoutComplete();
private:
    void startLogout(KWorkSpace::ShutdownType shutdownType);
    void runShutdownScripts();
    KWorkSpace::ShutdownType m_shutdownType;
};
