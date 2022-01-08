/*
    SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COMPONENTCHOOSERDATA_H
#define COMPONENTCHOOSERDATA_H

#include <QObject>

#include <KCModuleData>

class ComponentChooser;

class ComponentChooserData : public KCModuleData
{
    Q_OBJECT
public:
    ComponentChooserData(QObject *parent = nullptr, const QVariantList &args = QVariantList());

    void load();
    void save();
    void defaults();

    bool isDefaults() const override;
    bool isSaveNeeded() const;

    ComponentChooser *browsers() const;
    ComponentChooser *fileManagers() const;
    ComponentChooser *terminalEmulators() const;
    ComponentChooser *emailClients() const;
    ComponentChooser *geoUriHandlers() const;
    ComponentChooser *telUriHandlers() const;

private:
    ComponentChooser *m_browsers;
    ComponentChooser *m_fileManagers;
    ComponentChooser *m_terminalEmulators;
    ComponentChooser *m_emailClients;
    ComponentChooser *m_geoUriHandlers;
    ComponentChooser *m_telUriHandlers;
};

#endif // COMPONENTCHOOSERDATA_H
