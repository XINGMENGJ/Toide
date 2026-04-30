-- Toide: users (see collaborative-dev-platform-development-guide.md §5.2)
-- Apply: mysql -h 127.0.0.1 -u toide -p toide < server/migrations/001_users.sql

CREATE TABLE IF NOT EXISTS users (
  id CHAR(36) PRIMARY KEY,
  username VARCHAR(64) NOT NULL UNIQUE,
  email VARCHAR(255) NOT NULL UNIQUE,
  salt VARCHAR(128) NOT NULL,
  password_hash VARCHAR(128) NOT NULL,
  created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  last_login_at DATETIME(3) NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
