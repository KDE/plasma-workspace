/*****************************************************************

Copyright 2008 Christian Mollekopf <chrigi_1@hotmail.com>

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
#include "abstractgroupableitem.h"

#include <QWeakPointer>

#include <QDebug>

#include "taskgroup.h"
#include "taskmanager.h"


namespace TaskManager
{


class AbstractGroupableItem::Private
{
public:
    Private()
        : m_id(m_nextId++) {
    }

    QWeakPointer<TaskGroup> m_parentGroup;

    int m_id;

private:
    static int m_nextId;
};

int AbstractGroupableItem::Private::m_nextId = 1;


AbstractGroupableItem::AbstractGroupableItem(QObject *parent)
    :   QObject(parent),
        d(new Private)
{
}


AbstractGroupableItem::~AbstractGroupableItem()
{
    //kDebug();
    /*if (parentGroup()) {
        kDebug() << "Error: item gets destroyed but still has a parent group";
    }*/
    delete d;
}


bool AbstractGroupableItem::isGrouped() const
{
    return d->m_parentGroup && d->m_parentGroup.data()->parentGroup();
}

QIcon AbstractGroupableItem::icon() const
{
    return QIcon();
}

QString AbstractGroupableItem::name() const
{
    return QString();
}

QString AbstractGroupableItem::genericName() const
{
    return QString();
}

int AbstractGroupableItem::id() const
{
    return d->m_id;
}

WindowList AbstractGroupableItem::winIds() const
{
    return WindowList();
}

GroupPtr AbstractGroupableItem::parentGroup() const
{
    //kDebug();
    return d->m_parentGroup.data();
}


void AbstractGroupableItem::setParentGroup(const GroupPtr group)
{
    d->m_parentGroup = group;
}


//Item is member of group
bool AbstractGroupableItem::isGroupMember(const GroupPtr group) const
{
    //kDebug();
    if (!group) {
        //kDebug() << "Null Group Pointer";
        return false;
    }

    if (!parentGroup()) {
        return false;
    }

    return group->members().contains(const_cast<AbstractGroupableItem*>(this));
}

bool AbstractGroupableItem::isStartupItem() const
{
    return false;
}

} // TaskManager namespace

#include "abstractgroupableitem.moc"

