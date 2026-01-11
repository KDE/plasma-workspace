#pragma once

#include <KRunner/Action>
#include <KRunner/QueryMatch>
#include <QDBusArgument>
#include <QList>
#include <QString>
#include <QVariantMap>

struct RemoteMatch {
    // sssuda{sv}
    QString id;
    QString text;
    QString iconName;
    int categoryRelevance = qToUnderlying(KRunner::QueryMatch::CategoryRelevance::Lowest);
    qreal relevance = 0;
    QVariantMap properties;
};

typedef QList<RemoteMatch> RemoteMatches;

inline QDBusArgument &operator<<(QDBusArgument &argument, const RemoteMatch &match)
{
    argument.beginStructure();
    argument << match.id;
    argument << match.text;
    argument << match.iconName;
    argument << match.categoryRelevance;
    argument << match.relevance;
    argument << match.properties;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, RemoteMatch &match)
{
    argument.beginStructure();
    argument >> match.id;
    argument >> match.text;
    argument >> match.iconName;
    argument >> match.categoryRelevance;
    argument >> match.relevance;
    argument >> match.properties;
    argument.endStructure();

    return argument;
}

inline QDBusArgument &operator<<(QDBusArgument &argument, const KRunner::Action &action)
{
    argument.beginStructure();
    argument << action.id();
    argument << action.text();
    argument << action.iconSource();
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, KRunner::Action &action)
{
    QString id;
    QString text;
    QString iconName;
    argument.beginStructure();
    argument >> id;
    argument >> text;
    argument >> iconName;
    argument.endStructure();
    action = KRunner::Action(id, iconName, text);
    return argument;
}
