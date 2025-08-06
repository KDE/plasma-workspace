/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "openwithfilemanageraction.h"

#include <Solid/OpticalDrive>

#include <KService>

#include <QStandardPaths>

OpenWithFileManagerAction::OpenWithFileManagerAction(const std::shared_ptr<StorageInfo> &storageInfo,
                                                     const std::shared_ptr<StateInfo> &stateInfo,
                                                     QObject *parent)
    : ActionInterface(storageInfo, stateInfo, parent)
{
    const QString actionUrl = QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"solid/actions/" + predicate());
    auto services = KService(actionUrl).actions();
    if (services.isEmpty()) {
        m_isActionValid = false;
        return;
    }
    m_text = services[0].text();
    m_icon = services[0].icon();

    connect(m_stateInfo.get(), &StateInfo::stateChanged, this, &OpenWithFileManagerAction::updateIsValid);

    m_isActionValid = true;
}

OpenWithFileManagerAction::~OpenWithFileManagerAction() = default;

QString OpenWithFileManagerAction::predicate() const
{
    return QLatin1String("openWithFileManager.desktop");
}

bool OpenWithFileManagerAction::isValid() const
{
    return m_isActionValid && m_storageInfo->isRemovable() && m_stateInfo->isMounted();
}

QString OpenWithFileManagerAction::name() const
{
    return QStringLiteral("openWithFileManager");
}

QString OpenWithFileManagerAction::icon() const
{
    return m_icon;
}

QString OpenWithFileManagerAction::text() const
{
    return m_text;
}

void OpenWithFileManagerAction::updateIsValid(const QString &udi)
{
    Q_UNUSED(udi);

    Q_EMIT isValidChanged(name(), isValid());
}

#include "moc_openwithfilemanageraction.cpp"
