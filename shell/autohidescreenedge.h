/*
    SPDX-FileCopyrightText: 2023 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Plasma/Plasma>
#include <QPointer>

class PanelView;

class AutoHideScreenEdge : public QObject
{
    Q_OBJECT

public:
    static AutoHideScreenEdge *create(PanelView *view);

    virtual void hide() = 0;
    virtual void show() = 0;

protected:
    AutoHideScreenEdge(PanelView *view);

    QPointer<PanelView> m_view;
};
