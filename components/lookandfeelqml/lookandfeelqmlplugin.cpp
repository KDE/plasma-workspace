/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "lookandfeelqmlplugin.h"

#include "kpackageinterface.h"
#include <KConfigGroup>
#include <KPackage/PackageLoader>
#include <KSharedConfig>
#include <QQmlEngine>

using namespace Qt::StringLiterals;

void SessionsPrivatePlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.lookandfeel"));

    qmlRegisterSingletonType<KPackageInterface>(uri, 1, 0, "LookAndFeel", [](QQmlEngine *, QJSEngine *) {
        const KConfigGroup cg(KSharedConfig::openConfig(), u"KDE"_s);
        const auto packageName = cg.readEntry("LookAndFeelPackage", QString());
        const auto package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), packageName);
        return new KPackageInterface(package);
    });
}
