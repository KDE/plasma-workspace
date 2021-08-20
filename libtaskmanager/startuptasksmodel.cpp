/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "startuptasksmodel.h"
#include "abstracttasksmodel.h"

#include <config-X11.h>

#include "waylandstartuptasksmodel.h"
#if HAVE_X11
#include "xstartuptasksmodel.h"
#endif

#include <KWindowSystem>

namespace TaskManager
{
class Q_DECL_HIDDEN StartupTasksModel::Private
{
public:
    Private(StartupTasksModel *q);
    ~Private();

    static int instanceCount;
    static AbstractTasksModel *sourceTasksModel;

    void initSourceTasksModel();

private:
    StartupTasksModel *q;
};

int StartupTasksModel::Private::instanceCount = 0;
AbstractTasksModel *StartupTasksModel::Private::sourceTasksModel = nullptr;

StartupTasksModel::Private::Private(StartupTasksModel *q)
    : q(q)
{
    ++instanceCount;
}

StartupTasksModel::Private::~Private()
{
    --instanceCount;

    if (!instanceCount) {
        delete sourceTasksModel;
        sourceTasksModel = nullptr;
    }
}

void StartupTasksModel::Private::initSourceTasksModel()
{
    if (!sourceTasksModel && KWindowSystem::isPlatformWayland()) {
        sourceTasksModel = new WaylandStartupTasksModel();
    }

#if HAVE_X11
    if (!sourceTasksModel && KWindowSystem::isPlatformX11()) {
        sourceTasksModel = new XStartupTasksModel();
    }
#endif

    q->setSourceModel(sourceTasksModel);
}

StartupTasksModel::StartupTasksModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , d(new Private(this))
{
    d->initSourceTasksModel();
}

StartupTasksModel::~StartupTasksModel()
{
}

QHash<int, QByteArray> StartupTasksModel::roleNames() const
{
    if (d->sourceTasksModel) {
        return d->sourceTasksModel->roleNames();
    }

    return QHash<int, QByteArray>();
}

QModelIndex StartupTasksModel::mapIfaceToSource(const QModelIndex &index) const
{
    return mapToSource(index);
}

}
