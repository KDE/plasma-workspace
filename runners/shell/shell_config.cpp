
/***************************************************************************
 *   Copyright 2008 by Montel Laurent <montel@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "shell_config.h"


#include <KConfigGroup>
#include <QDebug>
#include <KPluginFactory>
#include <KPluginLoader>

#include <krunner/abstractrunner.h>

ShellConfig::ShellConfig(const KConfigGroup &config, QWidget* parent)
    : QWidget(parent),
      m_config(config)
{
    QHBoxLayout *hboxLayout = new QHBoxLayout(parent);
    hboxLayout->addWidget(this);
    m_ui.setupUi(this);

#ifdef Q_OS_UNIX
    connect(m_ui.cbRunAsOther, &QAbstractButton::clicked, this, &ShellConfig::slotUpdateUser);
#else
    m_ui.cbRunAsOther->hide();
    m_ui.lbUsername->hide();
    m_ui.leUsername->hide();
    m_ui.lbPassword->hide();
    m_ui.lePassword->hide();
#endif
}

ShellConfig::~ShellConfig()
{
}

void ShellConfig::slotUpdateUser(bool b)
{
    if (b) {
        m_ui.leUsername->setFocus();
    }
}

