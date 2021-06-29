/*
    SPDX-FileCopyrightText: 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "jobaction.h"

#include <QDebug>
#include <kio/global.h>
#include <klocalizedstring.h>

void JobAction::start()
{
    qDebug() << "Trying to perform the action" << operationName();

    if (!m_job) {
        setErrorText(i18nc("%1 is the subject (can be anything) upon which the job is performed", "The JobView for %1 cannot be found", destination()));
        setError(-1);
        emitResult();
        return;
    }

    // TODO: check with capabilities before performing actions.
    if (operationName() == QLatin1String("resume")) {
        m_job->resume();
    } else if (operationName() == QLatin1String("suspend")) {
        m_job->suspend();
    } else if (operationName() == QLatin1String("stop")) {
        m_job->kill();
    }

    emitResult();
}
