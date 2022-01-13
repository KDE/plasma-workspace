/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kpackageinterface.h"

KPackageInterface::KPackageInterface(const KPackage::Package &package)
    : m_package(package)
{
    Q_ASSERT(m_package.isValid());
}

QUrl KPackageInterface::fileUrl(const QByteArray &key) const
{
    return m_package.fileUrl(key);
}
