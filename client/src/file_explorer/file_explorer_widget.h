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
    [[nodiscard]] QString createFileInDirectory(const QString &directoryPath,
                                                const QString &name,
                                                QString *errorMessage = nullptr);
    [[nodiscard]] QString createFolderInDirectory(const QString &directoryPath,
                                                  const QString &name,
                                                  QString *errorMessage = nullptr);

public slots:
    void setProjectRoot(const QString &projectPath);
    /// Call after saving an existing path so the tree catches up if OS notifications were delayed.
    void noteFileChangedOnDisk(const QString &absoluteFilePath);

signals:
    void fileOpenRequested(const QString &filePath);

private:
    void handleDoubleClicked(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);
    void promptCreateFile();
    void promptCreateFolder();
    [[nodiscard]] QString selectedTargetDirectory() const;
    [[nodiscard]] static bool isValidEntryName(const QString &name);
    [[nodiscard]] static QString trimmedName(const QString &name);
    void refreshAfterMutation(const QString &parentDir, const QString &highlightPath);

    QFileSystemModel *model_ = nullptr;
    QTreeView *treeView_ = nullptr;
    QString rootPath_;
};
