-- Workspace file versions for collaboration / conflict detection (MVP)
-- Aligns with collaborative-dev-platform-development-guide.md file version concept.

CREATE TABLE IF NOT EXISTS workspace_file_versions (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  project_key VARCHAR(128) NOT NULL,
  file_path VARCHAR(1024) NOT NULL,
  version BIGINT NOT NULL,
  content MEDIUMTEXT NOT NULL,
  updated_by CHAR(36) NOT NULL,
  updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  UNIQUE KEY uq_workspace_file_ver (project_key, file_path, version),
  KEY idx_workspace_lookup (project_key, file_path)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
