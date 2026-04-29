#include "workspace/recent_project_store.h"

#include <QDir>
#include <QSettings>

RecentProjectStore::RecentProjectStore(QString settingsFilePath, int maxProjects)
    : settingsFilePath_(std::move(settingsFilePath))
    , maxProjects_(maxProjects)
{
}

QStringList RecentProjectStore::recentProjects() const
{
    QSettings settings(settingsFilePath_, QSettings::IniFormat);
    return settings.value(QStringLiteral("recentProjects")).toStringList();
}

void RecentProjectStore::addProject(const QString &projectPath)
{
    const auto normalizedPath = QDir::cleanPath(projectPath);
    if (normalizedPath.isEmpty()) {
        return;
    }

    auto projects = recentProjects();
    projects.removeAll(normalizedPath);
    projects.prepend(normalizedPath);

    while (projects.size() > maxProjects_) {
        projects.removeLast();
    }

    saveRecentProjects(projects);
}

void RecentProjectStore::saveRecentProjects(const QStringList &projectPaths)
{
    QSettings settings(settingsFilePath_, QSettings::IniFormat);
    settings.setValue(QStringLiteral("recentProjects"), projectPaths);
}
