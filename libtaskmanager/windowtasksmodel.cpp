/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "windowtasksmodel.h"

#include <config-X11.h>

#include "waylandtasksmodel.h"
#if HAVE_X11
#include "xwindowtasksmodel.h"
#endif

#include <KWindowSystem>

namespace TaskManager
{
class Q_DECL_HIDDEN WindowTasksModel::Private
{
public:
    Private(WindowTasksModel *q);
    ~Private();

    static int instanceCount;
    static AbstractTasksModel *sourceTasksModel;

    void initSourceTasksModel();

private:
    WindowTasksModel *q;
};

int WindowTasksModel::Private::instanceCount = 0;
AbstractTasksModel *WindowTasksModel::Private::sourceTasksModel = nullptr;

WindowTasksModel::Private::Private(WindowTasksModel *q)
    : q(q)
{
    ++instanceCount;
}

WindowTasksModel::Private::~Private()
{
    --instanceCount;

    if (!instanceCount) {
        delete sourceTasksModel;
        sourceTasksModel = nullptr;
    }
}

void WindowTasksModel::Private::initSourceTasksModel()
{
    if (!sourceTasksModel && KWindowSystem::isPlatformWayland()) {
        sourceTasksModel = new WaylandTasksModel();
    }

#if HAVE_X11
    if (!sourceTasksModel && KWindowSystem::isPlatformX11()) {
        sourceTasksModel = new XWindowTasksModel();
    }
#endif

    q->setSourceModel(sourceTasksModel);
}

WindowTasksModel::WindowTasksModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , d(new Private(this))
{
    d->initSourceTasksModel();
}

WindowTasksModel::~WindowTasksModel()
{
}

QHash<int, QByteArray> WindowTasksModel::roleNames() const
{
    if (d->sourceTasksModel) {
        return d->sourceTasksModel->roleNames();
    }

    return QHash<int, QByteArray>();
}

QModelIndex WindowTasksModel::mapIfaceToSource(const QModelIndex &index) const
{
    return mapToSource(index);
}

}
