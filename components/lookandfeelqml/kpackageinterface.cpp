/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kpackageinterface.h"

#include <KConfigGroup>
#include <KPackage/PackageLoader>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

KPackageInterface::KPackageInterface(const KPackage::Package &package)
    : m_package(package)
{
    Q_ASSERT(m_package.isValid());
}

QUrl KPackageInterface::fileUrl(const QByteArray &key) const
{
    return m_package.fileUrl(key);
}

QUrl KPackageInterface::fallbackFileUrl(const QByteArray &key) const
{
    return m_package.fallbackPackage().fileUrl(key);
}

KPackageInterface *KPackageInterface::create(QQmlEngine *, QJSEngine *)
{
    const KConfigGroup cg(KSharedConfig::openConfig(), u"KDE"_s);
    const auto packageName = cg.readEntry("LookAndFeelPackage", QString());
    const auto package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), packageName);
    return new KPackageInterface(package);
}

#include "moc_kpackageinterface.cpp"
