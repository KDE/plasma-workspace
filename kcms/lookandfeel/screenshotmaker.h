/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QObject>

class ScreenshotRequest : public QObject
{
    Q_OBJECT

public:
    explicit ScreenshotRequest(const QString &objectPath, QObject *parent = nullptr);

Q_SIGNALS:
    void taken(const QUrl &fileUrl);

private Q_SLOTS:
    void onResponse(uint result, const QVariantMap &data);
};

class ScreenshotMaker : public QObject
{
    Q_OBJECT

public:
    explicit ScreenshotMaker(QObject *parent = nullptr);

public Q_SLOTS:
    void take();

Q_SIGNALS:
    void accepted(const QUrl &fileUrl);

private:
    bool m_waiting = false;
};
