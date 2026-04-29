#pragma once

#include <QWidget>

class QFileSystemModel;
class QModelIndex;
class QTreeView;

class FileExplorerWidget final : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorerWidget(QWidget *parent = nullptr);

    [[nodiscard]] QString rootPath() const;

public slots:
    void setProjectRoot(const QString &projectPath);

signals:
    void fileOpenRequested(const QString &filePath);

private:
    void handleDoubleClicked(const QModelIndex &index);

    QFileSystemModel *model_ = nullptr;
    QTreeView *treeView_ = nullptr;
    QString rootPath_;
};
