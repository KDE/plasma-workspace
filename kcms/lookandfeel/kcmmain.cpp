/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kcm.h"
#include "lookandfeeldata.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(KCMLookandFeelFactory, "kcm_lookandfeel.json", registerPlugin<KCMLookandFeel>(); registerPlugin<LookAndFeelData>();)

#include "kcmmain.moc"
