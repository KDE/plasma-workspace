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
    , m_numUnicodeBlocks(0)
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
        for (m_numUnicodeBlocks = 0; !constUnicodeBlocks[m_numUnicodeBlocks].blockName.isEmpty(); ++m_numUnicodeBlocks) {
            items.append(i18n("Unicode Block: %1", constUnicodeBlocks[m_numUnicodeBlocks].blockName.toString()));
        }

        for (int i = 0; !constUnicodeScriptList[i].isEmpty(); ++i) {
            items.append(i18n("Unicode Script: %1", constUnicodeScriptList[i].toString()));
        }
        break;
    case ScriptsOnly:
        for (int i = 0; !constUnicodeScriptList[i].isEmpty(); ++i) {
            items.append(constUnicodeScriptList[i].toString());
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
    } else if (index < m_numUnicodeBlocks + 2) {
        list.append(CFcEngine::TRange(constUnicodeBlocks[index - 2].start, constUnicodeBlocks[index - 2].end));
    } else {
        int script(index - (2 + m_numUnicodeBlocks));

        for (int i = 0; constUnicodeScripts[i].scriptIndex >= 0; ++i) {
            if (constUnicodeScripts[i].scriptIndex == script) {
                list.append(CFcEngine::TRange(constUnicodeScripts[i].start, constUnicodeScripts[i].end));
            }
        }
    }

    Q_EMIT range(list);
}

}
