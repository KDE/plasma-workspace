/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "PreviewSelectAction.h"
#include "UnicodeBlocks.h"
#include "UnicodeScripts.h"

namespace KFI
{
CPreviewSelectAction::CPreviewSelectAction(QObject *parent, Mode mode)
    : KSelectAction(QIcon::fromTheme("character-set"), i18n("Preview Type"), parent)
    , itsNumUnicodeBlocks(0)
{
    setMode(mode);

    connect(this, SIGNAL(triggered(int)), SLOT(selected(int)));
}

void CPreviewSelectAction::setStd()
{
    setCurrentItem(0);
    selected(0);
}

void CPreviewSelectAction::setMode(Mode mode)
{
    QStringList items;

    items.append(i18n("Standard Preview"));
    items.append(i18n("All Characters"));

    switch (mode) {
    default:
    case Basic:
        break;
    case BlocksAndScripts:
        for (itsNumUnicodeBlocks = 0; constUnicodeBlocks[itsNumUnicodeBlocks].blockName; ++itsNumUnicodeBlocks) {
            items.append(i18n("Unicode Block: %1", i18n(constUnicodeBlocks[itsNumUnicodeBlocks].blockName)));
        }

        for (int i = 0; constUnicodeScriptList[i]; ++i) {
            items.append(i18n("Unicode Script: %1", i18n(constUnicodeScriptList[i])));
        }
        break;
    case ScriptsOnly:
        for (int i = 0; constUnicodeScriptList[i]; ++i) {
            items.append(i18n(constUnicodeScriptList[i]));
        }
    }

    setItems(items);
    setStd();
}

void CPreviewSelectAction::selected(int index)
{
    QList<CFcEngine::TRange> list;

    if (0 == index) {
        ;
    } else if (1 == index) {
        list.append(CFcEngine::TRange());
    } else if (index < itsNumUnicodeBlocks + 2) {
        list.append(CFcEngine::TRange(constUnicodeBlocks[index - 2].start, constUnicodeBlocks[index - 2].end));
    } else {
        int script(index - (2 + itsNumUnicodeBlocks));

        for (int i = 0; constUnicodeScripts[i].scriptIndex >= 0; ++i) {
            if (constUnicodeScripts[i].scriptIndex == script) {
                list.append(CFcEngine::TRange(constUnicodeScripts[i].start, constUnicodeScripts[i].end));
            }
        }
    }

    emit range(list);
}

}
