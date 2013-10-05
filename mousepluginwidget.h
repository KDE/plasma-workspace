/*
 *   Copyright (c) 2009 Chani Armitage <chani@kde.org>
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

#ifndef MOUSEPLUGINWIDGET_H
#define MOUSEPLUGINWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QToolButton>

#include <kconfiggroup.h>

#include <plasma/containmentactions.h>
#include "mouseinputbutton.h"

class QComboBox;
class QGridLayout;

class  MousePluginWidget : public QObject
{
    Q_OBJECT
public:
    MousePluginWidget(const QString &plugin, const QString &trigger, QGridLayout *layoutHack, QWidget *parent = 0);
    ~MousePluginWidget();

    void setTrigger(const QString &trigger);
    void prepareForSave();
    void save();

signals:
    void triggerChanged(const QString &oldTrigger, const QString &newTrigger);
    void configChanged(const QString &trigger);


private slots:
    void changeTrigger(const QString &oldTrigger, const QString& newTrigger);


    void configure();
    void acceptConfig();
    void rejectConfig();
    void showAbout();

private:
    void updateConfig(const QString &trigger);

    QComboBox *m_pluginList;
    QDialog *m_configDlg;

    KPluginInfo m_plugin;
    QWeakPointer<Plasma::ContainmentActions> m_pluginInstance;
    Plasma::Containment *m_containment;
    QString m_lastConfigLocation;
    KConfigGroup m_tempConfig;
    KConfig m_tempConfigParent;

};
#endif

