// Copyright (C) 2022 David Reondo <kde@david-redondo.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDXDGEXPORTERV2_H
#define QWAYLANDXDGEXPORTERV2_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qwayland-xdg-foreign-unstable-v2.h>

#include <QtWaylandClient/qtwaylandclientglobal.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandXdgExportedV2 : public QtWayland::zxdg_exported_v2
{
public:
    explicit QWaylandXdgExportedV2(::zxdg_exported_v2 *object);
    ~QWaylandXdgExportedV2() override;
    QString handle() const;

private:
    void zxdg_exported_v2_handle(const QString &handle) override;
    QString mHandle;
};

class QWaylandXdgExporterV2 : public QtWayland::zxdg_exporter_v2
{
public:
    QWaylandXdgExporterV2(wl_registry *registry, uint32_t id, int version);
    ~QWaylandXdgExporterV2() override;
    QWaylandXdgExportedV2 *exportToplevel(wl_surface *surface);
};

}

QT_END_NAMESPACE

#endif // QWAYLANDXDGEXPORTERV2_H
