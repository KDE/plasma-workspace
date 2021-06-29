/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#pragma once

#include <QUrl>

#include "modeljob.h"

class AddEditPlaceJob : public ModelJob
{
public:
    AddEditPlaceJob(KFilePlacesModel *model, QModelIndex index, const QVariantMap &parameters, QObject *parent = nullptr)
        : ModelJob(parent, model, index, (index.isValid() ? "Edit" : "Add"), parameters)
        , m_text(parameters[QStringLiteral("Name")].toString())
        , m_url(parameters[QStringLiteral("Url")].toUrl())
        , m_icon(parameters[QStringLiteral("Icon")].toString())
    {
    }

    void start() override
    {
        if (m_index.isValid()) {
            m_model->editPlace(m_index, m_text, m_url, m_icon);
        } else {
            m_model->addPlace(m_text, m_url, m_icon);
        }
    }

private:
    QString m_text;
    QUrl m_url;
    QString m_icon;
};

class RemovePlaceJob : public ModelJob
{
public:
    RemovePlaceJob(KFilePlacesModel *model, const QModelIndex &index, QObject *parent)
        : ModelJob(parent, model, index, QStringLiteral("Remove"))
    {
    }

    void start() override
    {
        m_model->removePlace(m_index);
    }
};

class ShowPlaceJob : public ModelJob
{
public:
    ShowPlaceJob(KFilePlacesModel *model, const QModelIndex &index, bool show = true, QObject *parent = nullptr)
        : ModelJob(parent, model, index, (show ? "Show" : "Hide"))
        , m_show(show)
    {
    }

    void start() override
    {
        m_model->setPlaceHidden(m_index, m_show);
    }

private:
    bool m_show;
};

class TeardownDeviceJob : public ModelJob
{
public:
    TeardownDeviceJob(KFilePlacesModel *model, const QModelIndex &index, QObject *parent = nullptr)
        : ModelJob(parent, model, index, QStringLiteral("Teardown Device"))
    {
    }

    void start() override
    {
        m_model->requestTeardown(m_index);
    }
};

#include "setupdevicejob.h"
