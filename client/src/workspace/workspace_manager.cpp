#include "workspace/workspace_manager.h"

#include <QDir>
#include <QFileInfo>

WorkspaceManager::WorkspaceManager(QObject *parent)
    : QObject(parent)
{
}

QString WorkspaceManager::currentProjectPath() const
{
    return currentProjectPath_;
}

QString WorkspaceManager::currentProjectName() const
{
    if (currentProjectPath_.isEmpty()) {
        return {};
    }

    return QFileInfo(currentProjectPath_).fileName();
}

bool WorkspaceManager::openProject(const QString &projectPath)
{
    const QFileInfo projectInfo(projectPath);
    if (!projectInfo.exists() || !projectInfo.isDir()) {
        return false;
    }

    currentProjectPath_ = QDir::cleanPath(projectInfo.absoluteFilePath());
    emit projectOpened(currentProjectPath_);
    return true;
}
