/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

class QQuickWindow;

/*
 * Caps the scene graph's texture atlas for one of the shell's own windows.
 *
 * Qt sizes that atlas from the window it belongs to, rounded up to a power of two, so a full screen window
 * reserves a screen sized sheet, and there is no API for it: the size is read from QSG_ATLAS_WIDTH and
 * QSG_ATLAS_HEIGHT when the window's scene graph starts, which happens on its render thread.
 *
 * So the variables are put in the environment while a window of ours is coming up and taken back out once
 * it has read them, rather than being set for the process: plasmashell launches applications, and they have
 * no business inheriting a shell's atlas policy.
 */
namespace SceneGraphAtlas
{
/*!
 * Puts the cap in the environment, and takes it out again once the window's scene graph has started.
 *
 * Windows coming up at the same time are counted, so the cap stays until the last of them has read it.
 */
void capFor(QQuickWindow *window);
}
