-- Toide: create database and application user (run as MySQL admin).
-- Example: mysql -u root -p < scripts/init-mysql.sql

CREATE DATABASE IF NOT EXISTS toide
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

CREATE USER IF NOT EXISTS 'toide'@'localhost' IDENTIFIED BY '';
GRANT ALL PRIVILEGES ON toide.* TO 'toide'@'localhost';
FLUSH PRIVILEGES;
