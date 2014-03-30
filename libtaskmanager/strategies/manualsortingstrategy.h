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

#ifndef MANUALSORTINGSTRATEGY_H
#define MANUALSORTINGSTRATEGY_H

#include "abstractsortingstrategy.h"

namespace TaskManager
{

/**
* Manual Sorting
* If showAllDesktops is enabled the position of the tasks logically changes on all desktops
* If showAllDesktops is disabled the position only changes per virtual desktop even
* if the task is on all desktops
*/

class ManualSortingStrategy : public AbstractSortingStrategy
{
    Q_OBJECT
public:
    ManualSortingStrategy(GroupManager *parent);
    ~ManualSortingStrategy();
    bool manualSortingRequest(AbstractGroupableItem *item, int newIndex);
    void sortItems(ItemList &items);
};


} // TaskManager namespace
#endif
