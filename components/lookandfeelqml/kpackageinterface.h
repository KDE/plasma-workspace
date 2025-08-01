/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <KPackage/Package>
#include <QObject>
#include <QQmlEngine>

class KPackageInterface : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LookAndFeel)
    QML_SINGLETON
public:
    KPackageInterface(const KPackage::Package &package);

    Q_INVOKABLE QUrl fileUrl(const QByteArray &key) const;
    Q_INVOKABLE QUrl fallbackFileUrl(const QByteArray &key) const;

    static KPackageInterface *create(QQmlEngine *, QJSEngine *);

private:
    KPackageInterface() = default;
    const KPackage::Package m_package;
};
