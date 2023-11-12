// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#include "defaultservice.h"

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KSharedConfig>

KService::Ptr DefaultService::browser()
{
    // NOTE this function could be lifted into a framework or refactored into only following scheme-handler
    KService::Ptr preferredService = KApplicationTrader::preferredService(QStringLiteral("x-scheme-handler/http"));
    if (preferredService) {
        return preferredService;
    }

    KService::Ptr htmlApp = KApplicationTrader::preferredService(QStringLiteral("text/html"));
    if (htmlApp) {
        return htmlApp;
    }

    return KService::serviceByStorageId(legacyBrowserExec());
}

QString DefaultService::legacyBrowserExec()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("General"));
    QString browserApp = config.readPathEntry("BrowserApplication", QString());
    if (!browserApp.isEmpty()) {
        if (browserApp.startsWith(QLatin1Char('!'))) {
            browserApp.remove(0, 1);
        }
        return browserApp;
    }

    return {};
}
