/*****************************************************************************
 *   Copyright (C) 2009 by Shaun Reich <shaun.reich@kdemail.net>             *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or           *
 *   modify it under the terms of the GNU General Public License as          *
 *   published by the Free Software Foundation; either version 2 of          *
 *   the License, or (at your option) any later version.                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/


#ifndef REQUESTVIEWCALLWATCHER_H
#define REQUESTVIEWCALLWATCHER_H

#include <QDBusPendingCallWatcher>
class JobView;

class RequestViewCallWatcher : public QDBusPendingCallWatcher
{
    Q_OBJECT

public:
    RequestViewCallWatcher(JobView* jobView, const QString& service, const QDBusPendingCall& call, QObject* parent);


    JobView* jobView() const {
        return m_jobView;
    }
    QString service() const {
        return m_service;
    }

signals:
    void callFinished(RequestViewCallWatcher*);

private slots:
    void slotFinished() {
        emit callFinished(this);
    }

private:
    JobView* m_jobView;
    QString m_service;
};

#endif /* REQUESTVIEWCALLWATCHER_H */
