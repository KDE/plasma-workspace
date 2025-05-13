#include "wallpaperurl.h"

WallpaperUrl::WallpaperUrl(QObject *parent)
    : QObject(parent)
{
}

QUrl WallpaperUrl::make(const QString &raw, const QString &fragment) const
{
    return make(QUrl::fromUserInput(raw), fragment);
}

QUrl WallpaperUrl::make(const QUrl &url, const QString &fragment) const
{
    QUrl ret = url;
    if (!fragment.isEmpty()) {
        ret.setFragment(fragment);
    }
    return ret;
}

QString WallpaperUrl::fragment(const QString &raw) const
{
    return fragment(QUrl::fromUserInput(raw));
}

QString WallpaperUrl::fragment(const QUrl &url) const
{
    return url.fragment();
}

#include "moc_wallpaperurl.cpp"
