#pragma once

#include <QObject>
#include <QUrl>

class WallpaperUrl : public QObject
{
    Q_OBJECT

public:
    explicit WallpaperUrl(QObject *parent = nullptr);

    Q_INVOKABLE QUrl make(const QString &raw, const QString &fragment = QString()) const;
    Q_INVOKABLE QUrl make(const QUrl &url, const QString &fragment = QString()) const;

    Q_INVOKABLE QString fragment(const QString &raw) const;
    Q_INVOKABLE QString fragment(const QUrl &url) const;
};
