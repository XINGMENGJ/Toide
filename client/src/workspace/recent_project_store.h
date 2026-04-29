#pragma once

#include <QString>
#include <QStringList>

class RecentProjectStore final {
public:
    explicit RecentProjectStore(QString settingsFilePath, int maxProjects = 10);

    [[nodiscard]] QStringList recentProjects() const;
    void addProject(const QString &projectPath);

private:
    void saveRecentProjects(const QStringList &projectPaths);

    QString settingsFilePath_;
    int maxProjects_ = 10;
};
