/***************************************************************************
                          componentchooseremail.h
                             -------------------
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundationi                            *
 *                                                                         *
 ***************************************************************************/

#ifndef COMPONENTCHOOSEREMAIL_H
#define COMPONENTCHOOSEREMAIL_H

class KEMailSettings;

#include "ui_emailclientconfig_ui.h"
#include "componentchooser.h"

class CfgEmailClient: public QWidget, public Ui::EmailClientConfig_UI, public CfgPlugin
{
    Q_OBJECT
public:
    CfgEmailClient(QWidget *parent);
    ~CfgEmailClient() override;
    void load(KConfig *cfg) override;
    void save(KConfig *cfg) override;
    void defaults() override;
    bool isDefaults() const override;

private:
    KEMailSettings *pSettings;
    KService::Ptr m_emailClientService;

protected Q_SLOTS:
    void selectEmailClient();
    void configChanged();
Q_SIGNALS:
    void changed(bool);
};

#endif
