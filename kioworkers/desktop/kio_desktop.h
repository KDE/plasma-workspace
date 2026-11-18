/*
    SPDX-FileCopyrightText: 2008, 2009 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KIO/ForwardingWorkerBase>

class DesktopProtocol : public KIO::ForwardingWorkerBase
{
    Q_OBJECT
public:
    DesktopProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app);
    ~DesktopProtocol() override;

protected:
    void checkLocalInstall();
    bool rewriteUrl(const QUrl &url, QUrl &newUrl) override;
};
