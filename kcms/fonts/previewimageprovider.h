/*
    SPDX-FileCopyrightText: 2018 Julian Wolff <wolff@julianwolff.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QFont>
#include <QQuickImageProvider>

class PreviewImageProvider : public QQuickImageProvider
{
public:
    PreviewImageProvider(const QFont &font);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QFont m_font;
};
