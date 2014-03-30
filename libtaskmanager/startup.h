/*****************************************************************

Copyright (c) 2000-2001 Matthias Elter <elter@kde.org>
Copyright (c) 2001 Richard Moore <rich@kde.org>

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

#ifndef STARTUP_H
#define STARTUP_H

// Qt
#include <QtCore/QObject>
#include <QtGui/QIcon>

// KDE
#include <KStartupInfo>

#include <taskmanager.h>
#include <taskmanager_export.h>

namespace TaskManager
{

/**
 * Represents a task which is in the process of starting.
 *
 * @see TaskManager
 */
class TASKMANAGER_EXPORT Startup: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text)
    Q_PROPERTY(QString bin READ bin)
    Q_PROPERTY(QIcon icon READ icon)

public:
    Startup(const KStartupInfoId& id, const KStartupInfoData& data, QObject * parent,
            const char *name = 0);
    virtual ~Startup();

    /**
     * The name of the starting task (if known).
     */
    QString text() const;
    QString desktopId() const;
    QString wmClass() const;

    /**
     * The name of the executable of the starting task.
     */
    QString bin() const;

    /**
     * The name of the icon to be used for the starting task.
     */
    QIcon icon() const;
    void update(const KStartupInfoData& data);
    KStartupInfoId id() const;

    void addWindowMatch(WId window);
    bool matchesWindow(WId window) const;

    /**
     * Releases pixmap objects; useful to clear out pixmap usage prior to application stoppage
     */
    void clearPixmapData();

Q_SIGNALS:
    /**
     * Indicates that this startup has changed in some way.
     */
    void changed(::TaskManager::TaskChanges);

private:
    class Private;
    Private * const d;
};

} // TaskManager namespace


#endif
