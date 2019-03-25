#include "slidemodel.h"

void SlideModel::reload(const QStringList &selected)
{
    if (!m_packages.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_packages.count() - 1);
        m_packages.clear();
        endRemoveRows();
        emit countChanged();
    }
    addDirs(selected);
}


void SlideModel::addDirs(const QStringList &selected)
{
    BackgroundFinder *finder = new BackgroundFinder(m_wallpaper.data(), selected);
    connect(finder, &BackgroundFinder::backgroundsFound, this, &SlideModel::backgroundsFound);
    m_findToken = finder->token();
    finder->start(); 
}

void SlideModel::backgroundsFound(const QStringList& paths, const QString& token)
{
     if (token != m_findToken) {
        return;
    }
    processPaths(paths);
}


void SlideModel::removeDir(const QString &path)
{
    BackgroundFinder *finder = new BackgroundFinder(m_wallpaper.data(), QStringList{path});
    connect(finder, &BackgroundFinder::backgroundsFound, this, &SlideModel::removeBackgrounds);
    finder->start();
}

void SlideModel::removeBackgrounds(const QStringList &paths, const QString &token)
{
    Q_FOREACH (const QString &file, paths) {
        removeBackground(file);
    }
}
