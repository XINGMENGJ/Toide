-- Registered server workspaces (project_key for file storage = workspaces.id)
CREATE TABLE IF NOT EXISTS workspaces (
  id CHAR(36) PRIMARY KEY,
  name VARCHAR(128) NOT NULL,
  created_by CHAR(36) NOT NULL,
  created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  KEY idx_workspaces_created_by (created_by),
  KEY idx_workspaces_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
