/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PASTE_HEADER
#define PASTE_HEADER

#include <plasma/containmentactions.h>

class QAction;

class Paste : public Plasma::ContainmentActions
{
    Q_OBJECT
public:
    Paste(QObject *parent, const QVariantList &args);

    QList<QAction *> contextualActions() override;

private Q_SLOTS:
    void doPaste();

private:
    QAction *m_action;
};

#endif
