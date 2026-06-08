#include "workspace/workspace_meta.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringLiteral>

QString WorkspaceMeta::workspaceJsonPath(const QString &workspaceRoot)
{
    return QDir(QDir::cleanPath(workspaceRoot)).filePath(QStringLiteral(".toide/workspace.json"));
}

std::optional<WorkspaceMeta> WorkspaceMeta::load(const QString &workspaceRoot)
{
    const QString path = workspaceJsonPath(workspaceRoot);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }
    QJsonParseError pe;
    const auto doc = QJsonDocument::fromJson(f.readAll(), &pe);
    f.close();
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }
    const auto o = doc.object();
    WorkspaceMeta m;
    m.serverWorkspaceId = o.value(QStringLiteral("serverWorkspaceId")).toString().trimmed();
    m.displayName = o.value(QStringLiteral("displayName")).toString().trimmed();
    if (m.serverWorkspaceId.isEmpty()) {
        return std::nullopt;
    }
    return m;
}

bool WorkspaceMeta::save(const QString &workspaceRoot, const WorkspaceMeta &meta)
{
    const QString dirPath = QDir(QDir::cleanPath(workspaceRoot)).filePath(QStringLiteral(".toide"));
    if (!QDir().mkpath(dirPath)) {
        return false;
    }
    QJsonObject o;
    o[QStringLiteral("serverWorkspaceId")] = meta.serverWorkspaceId;
    if (!meta.displayName.isEmpty()) {
        o[QStringLiteral("displayName")] = meta.displayName;
    }
    const QString path = workspaceJsonPath(workspaceRoot);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}
