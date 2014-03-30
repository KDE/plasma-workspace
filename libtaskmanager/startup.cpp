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

// Own
#include "startup.h"

// Qt
#include <QSet>

// libtaskmanager
#include "taskmanager.h"


namespace TaskManager
{

class Startup::Private
{
public:
    Private(const KStartupInfoId& id, const KStartupInfoData& data)
        : id(id), data(data) {
    }

    QIcon icon;
    KStartupInfoId id;
    KStartupInfoData data;
    QSet<WId> windowMatches;
};

Startup::Startup(const KStartupInfoId& id, const KStartupInfoData& data,
                 QObject * parent, const char *name)
    : QObject(parent),
      d(new Private(id, data))
{
    setObjectName(name);
}

Startup::~Startup()
{
    delete d;
}

QString Startup::text() const
{
    return d->data.findName();
}

QString Startup::desktopId() const
{
    return d->data.applicationId();
}

QString Startup::wmClass() const
{
    return d->data.WMClass();
}

QString Startup::bin() const
{
    return d->data.bin();
}

QIcon Startup::icon() const
{
    if (d->icon.isNull()) {
        d->icon = QIcon::fromTheme(d->data.findIcon());
    }

    return d->icon;
}

void Startup::update(const KStartupInfoData& data)
{
    d->data.update(data);
    emit changed(::TaskManager::TaskUnchanged);
}

KStartupInfoId Startup::id() const
{
    return d->id;
}

void Startup::addWindowMatch(WId window)
{
    d->windowMatches.insert(window);
}

bool Startup::matchesWindow(WId window) const
{
    return d->windowMatches.contains(window);
}

void Startup::clearPixmapData()
{
    d->icon = QIcon();
}

} // TaskManager namespace


#include "startup.moc"
