#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FcEngine.h"
#include <KSelectAction>

namespace KFI
{
class CPreviewSelectAction : public KSelectAction
{
    Q_OBJECT

public:
    enum Mode {
        Basic,
        BlocksAndScripts,
        ScriptsOnly,
    };

    explicit CPreviewSelectAction(QObject *parent, Mode mode = Basic);
    ~CPreviewSelectAction() override
    {
    }

    void setStd();
    void setMode(Mode mode);

Q_SIGNALS:

    void range(const QList<CFcEngine::TRange> &range);

private Q_SLOTS:

    void selected(int index);

private:
    int m_numUnicodeBlocks;
};

}
