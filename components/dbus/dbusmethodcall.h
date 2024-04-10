/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QBindable>
#include <QJSValue>
#include <QQmlParserStatus>
#include <QVariant>
#include <qqmlregistration.h>

#include "bustype.h"

class QDBusPendingCallWatcher;

class DBusMethodCall : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    /**
     * The bus type used in the method call
     */
    Q_PROPERTY(BusType::Type busType READ busType WRITE setBusType NOTIFY busTypeChanged)

    /**
     * The bus address of the method call
     */
    Q_PROPERTY(QString service READ service WRITE setService NOTIFY serviceChanged)

    /**
     * The object path of the method call is being sent to
     */
    Q_PROPERTY(QString objectPath READ objectPath WRITE setObjectPath NOTIFY objectPathChanged)

    /**
     * The interface of the method call
     */
    Q_PROPERTY(QString iface READ interface WRITE setInterface NOTIFY ifaceChanged)

    /**
     * The name of the method in the method call
     */
    Q_PROPERTY(QString method READ method WRITE setMethod NOTIFY methodChanged)

    /**
     * The input signature of the method call. Set it to @p undefined to enable automatic deduction.
     */
    Q_PROPERTY(QString inSignature READ inSignature WRITE setInSignature NOTIFY inSignatureChanged RESET resetInSignature)

    /**
     * The list of arguments that are going to be sent
     */
    Q_PROPERTY(QJSValue arguments READ arguments WRITE setArguments NOTIFY argumentsChanged)

public:
    explicit DBusMethodCall(QObject *parent = nullptr);
    ~DBusMethodCall() override;

    BusType::Type busType() const;
    void setBusType(BusType::Type type);

    QString service() const;
    void setService(const QString &value);

    QString objectPath() const;
    void setObjectPath(const QString &path);

    QString interface() const;
    void setInterface(const QString &iface);

    QString method() const;
    void setMethod(const QString &name);

    QString inSignature() const;
    void setInSignature(const QString &sig);
    void resetInSignature();

    QJSValue arguments() const;
    void setArguments(const QJSValue &args);

    Q_INVOKABLE void call();

Q_SIGNALS:
    void busTypeChanged();
    void serviceChanged();
    void objectPathChanged();
    void ifaceChanged();
    void methodChanged();
    void inSignatureChanged();
    void argumentsChanged();

    void success(const QVariantList &value);
    void error(const QString &name, const QString &message);

private:
    void classBegin() override;
    void componentComplete() override;

    void parseArguments();
    void parseSignatureFromIntrospection(QStringView introspection);
    void internalCall();
    void reset();

    bool m_ready = false;
    std::unique_ptr<QDBusPendingCallWatcher> m_watcher;
    std::unique_ptr<QDBusPendingCallWatcher> m_argParseWatcher;
    BusType::Type m_busType = BusType::Session;
    QString m_service;
    QString m_objectPath;
    QString m_interface;
    QString m_method;
    std::optional<QString> m_inSignature;
    bool m_isArgumentsSet = false;
    QJSValue m_arguments;
    QVariant m_argParseResult;
};
