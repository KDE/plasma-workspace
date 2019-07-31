/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef KEYBOARDLAYOUT_H
#define KEYBOARDLAYOUT_H

#include <QObject>
#include <QStringList>

class OrgKdeKeyboardLayoutsInterface;
class QDBusPendingCallWatcher;

class KeyboardLayout : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentLayout
               READ currentLayout
               WRITE setCurrentLayout
               NOTIFY currentLayoutChanged)

    Q_PROPERTY(QString currentLayoutDisplayName
               READ currentLayoutDisplayName
               NOTIFY currentLayoutDisplayNameChanged)

    Q_PROPERTY(QStringList layouts
               READ layouts
               NOTIFY layoutsChanged)

public:
    explicit KeyboardLayout(QObject *parent = nullptr);
    ~KeyboardLayout() override;

    QString currentLayout() const;
    QString currentLayoutDisplayName() const;
    QStringList layouts() const;

public Q_SLOTS:
    void setCurrentLayout(const QString &layout);

Q_SIGNALS:
    void currentLayoutChanged(const QString &newLayout);
    void currentLayoutDisplayNameChanged(const QString &newLayout);
    void layoutsChanged();

private Q_SLOTS:
    void requestCurrentLayout();
    void requestCurrentLayoutDisplayName();
    void requestLayoutsList();

    void onCurrentLayoutReceived(QDBusPendingCallWatcher *watcher);
    void onCurrentLayoutDisplayNameReceived(QDBusPendingCallWatcher *watcher);
    void onLayoutsListReceived(QDBusPendingCallWatcher *watcher);

private:
    QStringList mLayouts;
    QString mCurrentLayout;
    QString mCurrentLayoutDisplayName;
    OrgKdeKeyboardLayoutsInterface *mIface;

};


#endif // KEYBOARDLAYOUT_H
