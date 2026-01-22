/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

#include "storageinfo.h"

/**
 * Interface to add custom actions for devices
 */
class ActionInterface : public QObject
{
    Q_OBJECT

public:
    explicit ActionInterface(std::shared_ptr<StorageInfo> storageInfo, QObject *parent = nullptr);
    ~ActionInterface() override;

    /**
     * Must return name of predicate which will be triggered in solid/actions/
     * Can be not overridden if triggered is overridden
     */
    virtual QString predicate() const;

    /**
     * Method which starts when action is triggered
     * If not overridden then default implementation will start that triggers
     * predicate() in solid/actions/
     */
    virtual void triggered();

    /**
     * Name of action
     */
    virtual QString name() const = 0;

    /**
     * Icon of action
     */
    virtual QString icon() const = 0;

    /**
     * Text that describes action
     */
    virtual QString text() const = 0;

    /**
     * Check if the action is valid. If not then action will be ignored
     */
    virtual bool isValid() const;

Q_SIGNALS:

    /**
     * Emitted when text is changed
     */
    void textChanged(const QString &text);

    /**
     * Emitted when icon is changed
     */
    void iconChanged(const QString &icon);

    /**
     * Emitted when valid of action is changed
     */
    void isValidChanged(const QString &name, bool status);

protected:
    std::shared_ptr<StorageInfo> m_storageInfo;
};

#define ActionInterface_iid "com.plasma.private.ActionInterface"

Q_DECLARE_INTERFACE(ActionInterface, ActionInterface_iid)
