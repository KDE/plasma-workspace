/*
 * SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KXmlGuiWindow>

namespace KFI
{
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
};

}
