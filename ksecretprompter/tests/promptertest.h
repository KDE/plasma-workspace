/*
    SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class PrompterTest : public QObject
{
    Q_OBJECT
public:
    explicit PrompterTest(QObject *parent = nullptr);
    ~PrompterTest();

private:
};
