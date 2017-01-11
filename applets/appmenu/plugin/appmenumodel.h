/******************************************************************
 * Copyright 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************/

#include <QAbstractListModel>
#include <QStringList>
#include <KWindowSystem>
#include <QPointer>

class QMenu;
class QAction;
class QModelIndex;
class KDBusMenuImporter;

class AppMenuModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit AppMenuModel(QObject *parent = 0);
    ~AppMenuModel();

    enum AppMenuRole {
        MenuRole = Qt::UserRole+1,
        ActionRole
    };

    QVariant data(const QModelIndex &index, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QHash<int, QByteArray> roleNames() const;

    void updateApplicationMenu(const QString &serviceName, const QString &menuObjectPath);

private Q_SLOTS:
    void onActiveWindowChanged(WId id);
    void update();

signals:
    void modelNeedsUpdate();


private:
    bool m_winHasMenu;

    QPointer<QMenu> m_menu;
    QStringList m_activeMenu;
    QList<QAction *> m_activeActions;

    QString m_serviceName;
    QString m_menuObjectPath;

    QPointer<KDBusMenuImporter> m_importer;
};

