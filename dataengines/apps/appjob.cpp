/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "appjob.h"

#include <KIO/OpenUrlJob>

AppJob::AppJob(AppSource *source, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent)
    : ServiceJob(source->objectName(), operation, parameters, parent)
    , m_source(source)
{
}

AppJob::~AppJob()
{
}

void AppJob::start()
{
    const QString operation = operationName();
    if (operation == QLatin1String("launch")) {
        QString path = m_source->getApp()->entryPath();
        auto job = new KIO::OpenUrlJob(QUrl(path));
        job->start();
        setResult(true);
        return;
    }
    setResult(false);
}
