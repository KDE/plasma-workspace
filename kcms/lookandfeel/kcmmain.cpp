/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kcm.h"
#include "lookandfeeldata.h"

#include <KPluginFactory>

K_PLUGIN_CLASSES_WITH_JSON(KCMLookandFeel, LookAndFeelData, "kcm_lookandfeel.json")

#include "kcmmain.moc"
