/****************************************************************************
**
** Copyright (C) 2022 Marco Martin <mart@kde.org>
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCOMPOSITOR_PRIMARYOUTPUT_H
#define MOCKCOMPOSITOR_PRIMARYOUTPUT_H

#include "coreprotocol.h"
#include <qwayland-server-kde-primary-output-v1.h>

#include <QtGui/qpa/qplatformnativeinterface.h>

namespace MockCompositor
{
class PrimaryOutputV1 : public Global, public QtWaylandServer::kde_primary_output_v1
{
    Q_OBJECT
public:
    explicit PrimaryOutputV1(CoreCompositor *compositor, int version = 1);

    void setPrimaryOutputName(const QString &primaryName);

protected:
    void kde_primary_output_v1_bind_resource(Resource *resource) override;

private:
    QString m_primaryName;
};

} // namespace MockCompositor

#endif // MOCKCOMPOSITOR_PRIMARYOUTPUT_H
