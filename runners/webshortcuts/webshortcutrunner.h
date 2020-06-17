/*
 *   Copyright (C) 2007 Teemu Rytilahti <tpr@iki.fi>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef WEBSHORTCUTRUNNER_H
#define WEBSHORTCUTRUNNER_H

#include <KRunner/AbstractRunner>

class WebshortcutRunner : public Plasma::AbstractRunner 
{
    Q_OBJECT

    public:
        WebshortcutRunner(QObject *parent, const QVariantList& args);
        ~WebshortcutRunner() override;

        void match(Plasma::RunnerContext &context) override;
        QList<QAction *> actionsForMatch(const Plasma::QueryMatch &match) override;
        void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;

    private Q_SLOTS:
        void loadSyntaxes();
        void configurePrivateBrowsingActions();

    private:
        Plasma::QueryMatch m_match;
        bool m_filterBeforeRun;

        QChar m_delimiter;
        QString m_lastFailedKey;
        QString m_lastKey;
        QString m_lastProvider;
        
        KServiceAction m_privateAction;
};

#endif
