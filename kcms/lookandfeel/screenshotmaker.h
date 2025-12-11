/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QObject>

class ScreenshotMaker : public QObject
{
    Q_OBJECT

public:
    explicit ScreenshotMaker(QObject *parent = nullptr);

public Q_SLOTS:
    void take();

private Q_SLOTS:
    void onTaken(const QString &fileName);

Q_SIGNALS:
    void accepted(const QUrl &fileUrl);

private:
    bool m_waiting = false;
};
