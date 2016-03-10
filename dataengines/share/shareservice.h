/***************************************************************************
 *   Copyright 2010 Artur Duque de Souza <asouza@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef SHARE_SERVICE_H
#define SHARE_SERVICE_H

#include "shareengine.h"

#include <KPackage/Package>
#include <Plasma/Service>
#include <Plasma/ServiceJob>

class ShareProvider;

namespace Plasma {
    class ServiceJob;
}

namespace KJSEmbed {
    class Engine;
}

class ShareService : public Plasma::Service
{
    Q_OBJECT

public:
    ShareService(ShareEngine *engine);
    Plasma::ServiceJob *createJob(const QString &operation,
                                  QMap<QString, QVariant> &parameters) override;
};

class ShareJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    ShareJob(const QString &destination, const QString &operation,
             QMap<QString, QVariant> &parameters, QObject *parent = 0);
    ~ShareJob() override;
    void start() override;

public Q_SLOTS:
    void publish();
    void showResult(const QString &url);
    void showError(const QString &msg);

private:
    QScopedPointer<KJSEmbed::Engine> m_engine;
    ShareProvider *m_provider;
    KPackage::Package m_package;
};

#endif // SHARE_SERVICE
