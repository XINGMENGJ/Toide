#pragma once

#include <QObject>
#include <QString>

class WorkspaceManager final : public QObject {
    Q_OBJECT

public:
    explicit WorkspaceManager(QObject *parent = nullptr);

    [[nodiscard]] QString currentProjectPath() const;
    [[nodiscard]] QString currentProjectName() const;

    bool openProject(const QString &projectPath);
    static QString findDefaultExampleWorkspace(const QStringList &searchRoots);

signals:
    void projectOpened(const QString &projectPath);

private:
    QString currentProjectPath_;
};
