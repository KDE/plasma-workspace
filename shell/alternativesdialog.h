/*
 *   Copyright 2014 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
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

#ifndef ALTERNATIVESDIALOG_H
#define ALTERNATIVESDIALOG_H

#include <Plasma/Applet>

#include <plasmaquick/dialog.h>

namespace KDeclarative {
    class QmlObject;
}

//FIXME: this thing really ought to be in the shell
class AlternativesDialog : public PlasmaQuick::Dialog
{
    Q_OBJECT
    Q_PROPERTY(QStringList appletProvides READ appletProvides CONSTANT)
    Q_PROPERTY(QString currentPlugin READ currentPlugin CONSTANT)

public:
    AlternativesDialog(Plasma::Applet *applet, QQuickItem *parent = 0);
    ~AlternativesDialog();

    QStringList appletProvides() const;
    QString currentPlugin() const;

    Q_INVOKABLE void loadAlternative(const QString &plugin);

protected:
    void hideEvent(QHideEvent *ev);

private:
    Plasma::Applet *m_applet;
    KDeclarative::QmlObject *m_qmlObj;
};

#endif
