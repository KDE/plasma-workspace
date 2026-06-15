/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "windowtasksmodel.h"

#include <config-X11.h>

#include "waylandtasksmodel.h"

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
    WindowTasksModel *const q;
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
        delete std::exchange(sourceTasksModel, nullptr);
    }
}

void WindowTasksModel::Private::initSourceTasksModel()
{
    if (!sourceTasksModel) {
        sourceTasksModel = new WaylandTasksModel();
    }

    q->setSourceModel(sourceTasksModel);
}

WindowTasksModel::WindowTasksModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , d(new Private(this))
{
    d->initSourceTasksModel();
}

WindowTasksModel::~WindowTasksModel() = default;

QHash<int, QByteArray> WindowTasksModel::roleNames() const
{
    if (d->sourceTasksModel) {
        return d->sourceTasksModel->roleNames();
    }

    return {};
}

QModelIndex WindowTasksModel::mapIfaceToSource(const QModelIndex &index) const
{
    return mapToSource(index);
}

}

#include "moc_windowtasksmodel.cpp"
