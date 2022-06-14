/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

class QQuickWindow;
class QRectF;

namespace WideImage
{
QRectF cropRect(QQuickWindow *targetWindow, const QRectF &boundingRect);
}
