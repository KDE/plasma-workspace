/*****************************************************************
ksmserver - the KDE session management server

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

Copyright 2000 Matthias Ettrich <ettrich@kde.org>
******************************************************************/

#ifndef KSMSERVER_INTERFACE_H
#define KSMSERVER_INTERFACE_H

#include <dcopobject.h>

class KSMServerInterface : public QObject
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.KSMServerInterface")

public Q_SLOTS:
  Q_SCRIPTABLE void logout(int, int, int );
  Q_SCRIPTABLE QStringList sessionList();

  Q_SCRIPTABLE QString currentSession();
  Q_SCRIPTABLE void saveCurrentSession();
  Q_SCRIPTABLE void saveCurrentSessionAs( QString );

  Q_SCRIPTABLE void suspendStartup( QString );
  Q_SCRIPTABLE void resumeStartup( QString );
};

#endif
