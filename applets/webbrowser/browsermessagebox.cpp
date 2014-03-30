/***************************************************************************
 *   Copyright (C) 2010 Davide Bettio <davide.bettio@kdemail.net>          *
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

#include "browsermessagebox.h"

#include <QGraphicsLinearLayout>

#include <Plasma/Label>
#include <Plasma/PushButton>
#include <Plasma/WebView>

BrowserMessageBox::BrowserMessageBox(QGraphicsWidget *parent, QString message)
    : QGraphicsWidget(parent)
{
      QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(this);
      layout->setOrientation(Qt::Horizontal);

      Plasma::Label *messageLabel = new Plasma::Label(this);
      messageLabel->setText(message);
      layout->addItem(messageLabel);
      
      m_okButton = new Plasma::PushButton(this);
      m_okButton->setText(i18n("OK"));
      connect(m_okButton, SIGNAL(clicked()), this, SIGNAL(okClicked()));
      layout->addItem(m_okButton);
      
      m_cancelButton = new Plasma::PushButton(this);
      m_cancelButton->setText(i18n("Cancel"));
      connect(m_cancelButton, SIGNAL(clicked()), this, SIGNAL(cancelClicked()));
      layout->addItem(m_cancelButton);
}

Plasma::PushButton *BrowserMessageBox::okButton()
{
    return m_okButton;
}

Plasma::PushButton *BrowserMessageBox::cancelButton()
{
    return m_cancelButton;
}

#include "browsermessagebox.moc"
