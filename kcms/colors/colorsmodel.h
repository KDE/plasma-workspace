/*
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>
    SPDX-FileCopyrightText: 2016 Olivier Churlaud <olivier@churlaud.com>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QAbstractListModel>
#include <QPalette>
#include <QString>
#include <QVector>

struct ColorsModelData {
    QString display;
    QString schemeName;
    QPalette palette;
    QColor activeTitleBarBackground;
    QColor activeTitleBarForeground;
    bool removable;
    bool accentActiveTitlebar;
    bool pendingDeletion;
    bool tints;
    qreal tintFactor;
};
Q_DECLARE_TYPEINFO(ColorsModelData, Q_MOVABLE_TYPE);

class ColorsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString selectedScheme READ selectedScheme WRITE setSelectedScheme NOTIFY selectedSchemeChanged)
    Q_PROPERTY(int selectedSchemeIndex READ selectedSchemeIndex NOTIFY selectedSchemeIndexChanged)

public:
    ColorsModel(QObject *parent);
    ~ColorsModel() override;

    enum Roles {
        SchemeNameRole = Qt::UserRole + 1,
        PaletteRole,
        // Colors which aren't in QPalette
        ActiveTitleBarBackgroundRole,
        ActiveTitleBarForegroundRole,
        DisabledText,
        RemovableRole,
        AccentActiveTitlebarRole,
        PendingDeletionRole,
        Tints,
        TintFactor,
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    QString selectedScheme() const;
    void setSelectedScheme(const QString &scheme);

    int indexOfScheme(const QString &scheme) const;
    int selectedSchemeIndex() const;

    QStringList pendingDeletions() const;
    void removeItemsPendingDeletion();

    void load();

Q_SIGNALS:
    void selectedSchemeChanged(const QString &scheme);
    void selectedSchemeIndexChanged();

    void pendingDeletionsChanged();

private:
    QString m_selectedScheme;

    QVector<ColorsModelData> m_data;
};
