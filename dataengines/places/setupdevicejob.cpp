/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "setupdevicejob.h"

void SetupDeviceJob::setupDone(const QModelIndex &index, bool success)
{
    if (index == m_index) {
        setError(!success);
        emitResult();
    }
}

void SetupDeviceJob::setupError(const QString &message)
{
    if (!error() || errorText().isEmpty()) {
        setErrorText(message);
    }
}

// vim: sw=4 sts=4 et tw=100
