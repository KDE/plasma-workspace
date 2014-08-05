/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright (C) 2014 Marco Martin <mart@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include <KCModule>

#include "../../../lookandfeelaccess/lookandfeelaccess.h"

class QQuickWidget;
class QStandardItemModel;

class ScreenLockerKcm : public KCModule
{
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel *lockerModel READ lockerModel CONSTANT)
    Q_PROPERTY(QString selectedPlugin READ selectedPlugin WRITE setSelectedPlugin NOTIFY selectedPluginChanged)

public:
    enum Roles {
        PluginNameRole = Qt::UserRole +1,
        ScreenhotRole
    };
    explicit ScreenLockerKcm(QWidget *parent = nullptr, const QVariantList& args = QVariantList());

    QStandardItemModel *lockerModel();

    QString selectedPlugin() const;
    void setSelectedPlugin(const QString &plugin);

public Q_SLOTS:
    void load();
    void save() override;
    void test(const QString &plugin);

Q_SIGNALS:
    void selectedPluginChanged();

private:
    QStandardItemModel *m_model;
    QString m_selectedPlugin;
    QQuickWidget *m_quickWidget;
    LookAndFeelAccess m_access;
};
