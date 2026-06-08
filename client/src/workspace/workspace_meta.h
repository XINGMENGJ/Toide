#pragma once

#include <optional>
#include <QString>

struct WorkspaceMeta {
    QString serverWorkspaceId;
    QString displayName;

    [[nodiscard]] static QString workspaceJsonPath(const QString &workspaceRoot);
    [[nodiscard]] static std::optional<WorkspaceMeta> load(const QString &workspaceRoot);
    static bool save(const QString &workspaceRoot, const WorkspaceMeta &meta);
};
