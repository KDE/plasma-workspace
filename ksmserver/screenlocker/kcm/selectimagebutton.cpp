/*
 * Button for selecting an image.
 *
 * Copyright (C) 2011  Martin Klapetek <martin.klapetek@gmail.com>
 * Copyright (C) 2011, 2012, 2014 David Edmundson <kde@davidedmundson.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "selectimagebutton.h"

#include <QWidgetAction>

#include <QUrl>
#include <QFileDialog>
#include <QMenu>

#include <KLocalizedString>
#include <KMessageBox>

SelectImageButton::SelectImageButton(QWidget *parent)
    : QToolButton(parent)
{
    QMenu *menu = new QMenu(this);

    setPopupMode(QToolButton::InstantPopup);

    setIconSize(QSize(64,64));

    menu->addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("Load from file..."), this, SLOT(onLoadImageFromFile()));
    menu->addAction(QIcon::fromTheme(QStringLiteral("edit-clear")), i18n("Clear Image"), this, SLOT(onClearImage()));
    setMenu(menu);

    onClearImage();
}

SelectImageButton::~SelectImageButton() = default;

void SelectImageButton::setImagePath(const QString &imagePath)
{
    m_imagePath = imagePath;

    QPixmap image(imagePath);
    if (!image.isNull()) {
        QIcon imageIcon;
        //scale oversized avatars to fit, but don't stretch smaller images
        imageIcon.addPixmap(image.scaled(iconSize().boundedTo(image.size()), Qt::KeepAspectRatio));
        setIcon(imageIcon);
    } else {
        setIcon(QIcon::fromTheme(QStringLiteral("image-x-generic")));
    }
    Q_EMIT imagePathChanged(m_imagePath);
}

QString SelectImageButton::imagePath() const
{
    return m_imagePath;
}


void SelectImageButton::onLoadImageFromFile()
{
    QUrl fileUrl = QFileDialog::getOpenFileUrl(this, i18n("Select image"), QUrl(), QStringLiteral("image/*"), 0, 0, QStringList() << QStringLiteral("file"));

    if (!fileUrl.isEmpty()) {
        setImagePath(fileUrl.path());
    }
}

void SelectImageButton::onClearImage()
{
    setImagePath(QString());
}
