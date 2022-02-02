/****************************************************************************
**
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

#ifndef MOCKCOMPOSITOR_TEXTINPUT_H
#define MOCKCOMPOSITOR_TEXTINPUT_H

#include "coreprotocol.h"
#include <qwayland-server-text-input-unstable-v2.h>

#include <QtGui/qpa/qplatformnativeinterface.h>

namespace MockCompositor
{
class TextInputManager : public Global, public QtWaylandServer::zwp_text_input_manager_v2
{
    Q_OBJECT
public:
    TextInputManager(CoreCompositor *compositor);

protected:
    void zwp_text_input_manager_v2_get_text_input(Resource *resource, uint32_t id, struct ::wl_resource *seatResource) override;
};

} // namespace MockCompositor

#endif // MOCKCOMPOSITOR_TEXTINPUT_H
