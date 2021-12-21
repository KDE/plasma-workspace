/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <KPackage/Package>
#include <QObject>

class KPackageInterface : public QObject
{
    Q_OBJECT
public:
    KPackageInterface(const KPackage::Package &package);

    Q_INVOKABLE QUrl fileUrl(const QByteArray &key) const;

private:
    const KPackage::Package m_package;
};
